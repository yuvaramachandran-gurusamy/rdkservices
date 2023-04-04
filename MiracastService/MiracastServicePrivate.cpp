/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <MiracastServicePrivate.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <stdio.h>
#include "libIBus.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fstream>

extern void GStreamerThreadFunc(void *);

#define REMOVE_SPACES(x) x.erase(std::remove(x.begin(), x.end(), ' '), x.end())
#define REMOVE_R(x) x.erase(std::remove(x.begin(), x.end(), '\r'), x.end())
#define REMOVE_N(x) x.erase(std::remove(x.begin(), x.end(), '\n'), x.end())

#define MAX_EPOLL_EVENTS 64
#define SPACE " "

#define ENABLE_NON_BLOCKING
#define ONE_SECOND_PER_MILLISEC		( 1000 )
#define SOCKET_WAIT_TIMEOUT_IN_MILLISEC	( 30 * ONE_SECOND_PER_MILLISEC )

static MiracastPrivate* g_miracastPrivate = NULL;
std::string dummy = "";

RTSP_MSG_TEMPLATE_INFO MiracastRTSPMessages::rtsp_msg_template_info[] = {
	{ RTSP_MSG_FMT_M1_RESPONSE , "RTSP/1.0 200 OK\r\nPublic: \"%s, GET_PARAMETER, SET_PARAMETER\"\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M2_REQUEST , "OPTIONS * RTSP/1.0\r\nRequire: %s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M3_RESPONSE , "RTSP/1.0 200 OK\r\nContent-Length: %s\r\nContent-Type: text/parameters\r\nCSeq: %s\r\n\r\n%s" },
	{ RTSP_MSG_FMT_M4_RESPONSE , "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M5_RESPONSE , "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M6_REQUEST , "SETUP %s RTSP/1.0\r\nTransport: %s;%sclient_port=%s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M7_REQUEST , "PLAY %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_M16_RESPONSE , "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_PAUSE_REQUEST , "PAUSE %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_PLAY_REQUEST , "PLAY %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_TEARDOWN_REQUEST , "TEARDOWN %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n" },
	{ RTSP_MSG_FMT_TEARDOWN_RESPONSE , "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n" }
};

void ClientRequestHandlerCallback( void* args );
void SessionMgrThreadCallback( void* args );
void RTSPMsgHandlerCallback( void* args );

std::string MiracastPrivate::storeData(const char* tmpBuff, const char* lookup_data)
{
    char return_buf[1024] = {0};
    const char* ret = NULL, *ret_equal = NULL, *ret_space = NULL;
    ret = strstr(tmpBuff, lookup_data);
    if (NULL != ret)
    {
        if(0 == strncmp(ret, lookup_data, strlen(lookup_data)))
        {
            ret_equal = strstr(ret, "=");
            ret_space = strstr(ret_equal, " ");
            if(ret_space)
            {
                snprintf(return_buf, (int)(ret_space - ret_equal), "%s", ret+strlen(lookup_data)+1);
                MIRACASTLOG_VERBOSE("Parsed Data is - %s", return_buf);
            }
            else
            {
                snprintf(return_buf, strlen(ret_equal-1), "%s", ret+strlen(lookup_data)+1);
                MIRACASTLOG_VERBOSE("Parsed Data is - %s", return_buf);
            }
        }
    }
    if(return_buf != NULL)
        return std::string(return_buf);
    else
        return std::string(" ");
}

std::string MiracastPrivate::startDHCPClient(std::string interface)
{
    FILE *fp;
    std::string IP;
    char command[128] = {0};
    char data[1024] = {0};

    size_t len = 0;
    char *ln = NULL;

    sprintf(command, "/sbin/udhcpc -i ");
    sprintf(command+strlen(command), interface.c_str());
    sprintf(command+strlen(command), " 2>&1 | grep -i \"lease of\" | cut -d ' ' -f 4");

    MIRACASTLOG_VERBOSE("command : [%s]", command);

    fp = popen(command,"r");

    if(!fp){
        MIRACASTLOG_ERROR("Could not open pipe for output.");
        return std::string();
    }

    while (getline(&ln, &len, fp) != -1)
    {
        sprintf(data+strlen(data), ln);
        MIRACASTLOG_VERBOSE("data : [%s]", data);
        fputs(ln, stdout);
    }
    std::string local_addr(data);
    MIRACASTLOG_VERBOSE("local IP addr obtained is %s", local_addr.c_str());
    REMOVE_SPACES(local_addr);
    free(ln);
    return local_addr;
}

#if 0
bool MiracastPrivate::connectSink()
{
    std::string response;
    char buf[4096] = {0};
    size_t found;
    MIRACASTLOG_INFO("sink established. Commencing RTSP transactions.");

    int read_ret;
    size_t buf_size = sizeof(buf);

    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read ");
    }

    std::string buffer = buf;
    found = buffer.find("OPTIONS");
    MIRACASTLOG_INFO("received length is %d", read_ret);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("returning as nothing is received");
        return false;
    }
    MIRACASTLOG_INFO("received string(%d) - %s", buffer.length(), buffer.c_str());
    MIRACASTLOG_INFO("M1 request received");

    if(found!=std::string::npos)
    {
        MIRACASTLOG_INFO("M1 OPTIONS packet received");
        size_t found_str = buffer.find("Require");
        std::string req_str;
        if(found_str != std::string::npos)
        {
            req_str = buffer.substr(found_str+9);
            REMOVE_R(req_str);
            REMOVE_N(req_str);
        }
        response = "RTSP/1.0 200 OK\r\nPublic: \"";
        response.append(req_str);
        response.append(", GET_PARAMETER, SET_PARAMETER\"\r\nCSeq: 1\r\n\r\n");
        MIRACASTLOG_INFO("%s", response.c_str());
        read_ret = 0;
        read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
        if(read_ret < 0)
        {
            MIRACASTLOG_INFO("write error received (%d)%s", errno, strerror(errno));
            return false;
        }

        MIRACASTLOG_INFO("Sending the M1 response %d", read_ret);
    }
    std::string m2_msg("OPTIONS * RTSP/1.0\r\nRequire: org.wfa.wfd1.0\r\nCSeq: 1\r\n\r\n");
    MIRACASTLOG_INFO("%s", m2_msg.c_str());
    read_ret = 0;
    read_ret = send(m_tcpSockfd, m2_msg.c_str(), m2_msg.length(), 0);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("write error received (%d)%s", errno, strerror(errno));
        return false;
    }

    MIRACASTLOG_INFO("Sending the M2 request %d", read_ret);

    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M2 resp)");
        return false;
    }
    MIRACASTLOG_INFO("M2 response received");
    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M3 req)");
        return false;
    }
    MIRACASTLOG_INFO("M3 request received");
    buffer.clear();
    buffer = buf;
    if(buffer.find("wfd_video_formats") != std::string::npos)
    {
        MIRACASTLOG_INFO("In else case ");
        response.clear();
        response = "RTSP/1.0 200 OK\r\nContent-Length: 210\r\nContent-Type: text/parameters\r\nCSeq: 2\r\n\r\nwfd_content_protection: none\r\nwfd_video_formats: 00 00 03 10 0001ffff 1fffffff 00001fff 00 0000 0000 10 none none\r\nwfd_audio_codecs: AAC 00000007 00\r\nwfd_client_rtp_ports: RTP/AVP/UDP;unicast 1990 0 mode=play\r\n";
        MIRACASTLOG_INFO("%s", response.c_str());
        read_ret = 0;
        read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
        if(read_ret < 0)
        {
            MIRACASTLOG_INFO("write error received (%d)(%s)", errno, strerror(errno));
            return false;
        }
        MIRACASTLOG_INFO("Sending the M3 response %d", read_ret);
    }
    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M4 req)");
        return false;
    }
    MIRACASTLOG_INFO("M4 packet received");
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("returning as nothing is received");
        return false;
    }
    buffer.clear();
    buffer = buf;
    if(buffer.find("SET_PARAMETER") != std::string::npos)
    {
        response.clear();
        response = "RTSP/1.0 200 OK\r\nCSeq: 3\r\n\r\n";
        MIRACASTLOG_INFO("%s", response.c_str());
        read_ret = 0;
        read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
        if(read_ret < 0)
        {
            MIRACASTLOG_INFO("write error received (%d)(%s)", errno, strerror(errno));
            return false;
        }
        MIRACASTLOG_INFO("Sending the M4 response %d", read_ret);

    }
    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M5 req)");
        return false;
    }
    MIRACASTLOG_INFO("M5 packet received");
    buffer.clear();
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("returning as nothing is received");
        return false;
    }
    buffer = buf;
    if(buffer.find("wfd_trigger_method: SETUP") != std::string::npos)
    {
        response.clear();
        response = "RTSP/1.0 200 OK\r\nCSeq: 4\r\n\r\n";
        MIRACASTLOG_INFO("%s", response.c_str());
        read_ret = 0;
        read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
        if(read_ret < 0)
        {
            MIRACASTLOG_INFO("write error received (%d) (%s)", errno, strerror(errno));
            return false;
        }
        MIRACASTLOG_INFO("Sending the M5 response %d", read_ret);
    }

    response.clear();
    response = "SETUP rtsp://192.168.49.1/wfd1.0/streamid=0 RTSP/1.0\r\nTransport: RTP/AVP/UDP;unicast;client_port=1990\r\nCSeq: 2\r\n\r\n";
    MIRACASTLOG_INFO("%s", response.c_str());
    read_ret = 0;
    read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("write error received (%d) (%s)", errno, strerror(errno));
        return false;
    }
    MIRACASTLOG_INFO("Sending the M6 request %d", read_ret);

    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M6 resp)");
        return false;
    }
    MIRACASTLOG_INFO("M6 packet(resp) received");
    buffer.clear();
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("returning as nothing is received");
        return false;
    }
    buffer = buf;
    size_t pos_ses = buffer.find("Session");
    std::string session = buffer.substr(pos_ses+strlen("Session: "));
    pos_ses = session.find(";");
    std::string session_number = session.substr(0, pos_ses);

    if(buffer.find("client_port") != std::string::npos)
    {
        response.clear();

        response = "PLAY rtsp://192.168.49.1/wfd1.0/streamid=0 RTSP/1.0\r\nSession: ";
        response.append(session_number);
        response.append("\r\nCSeq: 3\r\n\r\n");
        MIRACASTLOG_INFO("%s", response.c_str());
        read_ret = 0;
        read_ret = send(m_tcpSockfd, response.c_str(), response.length(), 0);
        if(read_ret < 0)
        {
            MIRACASTLOG_INFO("write error received (%d) (%s)", errno, strerror(errno));
            return false;
        }
        MIRACASTLOG_INFO("Sending the M7 request %d", read_ret);
    }
    bzero(buf, sizeof(buf));
    read_ret = read(m_tcpSockfd, buf, buf_size-1);
    if(read_ret < 0)
    {
        MIRACASTLOG_INFO("read error received (%d)%s", errno, strerror(errno));
        return false;
    }

    if(strlen(buf) > 1)
    {
        MIRACASTLOG_INFO("Buffer read is - %s ", buf);
    }
    else
    {
        MIRACASTLOG_INFO("Buffer read is empty (M7 resp)");
        return false;
    }
    MIRACASTLOG_INFO("M7 packet(resp) received");
    return TRUE;
}
#endif

/*
 * Wait for data returned by the socket for specified time
 */
bool MiracastPrivate::waitDataTimeout( int m_Sockfd , unsigned ms)
{
	struct timeval timeout;
	fd_set readFDSet;

	FD_ZERO(&readFDSet);
	FD_SET( m_Sockfd , &readFDSet );

	timeout.tv_sec = (ms / 1000);
	timeout.tv_usec = ((ms % 1000) * 1000);

	if (select( m_Sockfd + 1, &readFDSet, NULL, NULL, &timeout) > 0)
	{
		return FD_ISSET( m_Sockfd , &readFDSet);
	}
	return false;
}

bool MiracastPrivate::ReceiveBufferTimedOut( void* buffer , size_t buffer_len )
{
	int recv_return = 0;
	bool status = true;

#ifdef ENABLE_NON_BLOCKING
	if (!waitDataTimeout( m_tcpSockfd , SOCKET_WAIT_TIMEOUT_IN_MILLISEC ))
	{
		recv_return = -1;
	}
	else
	{
		recv_return = recv( m_tcpSockfd , buffer, buffer_len , 0 );
	}
#else
	recv_return = read(m_tcpSockfd, buffer , buffer_len );
#endif
	if ( recv_return <= 0 ){
		status = false;

		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			MIRACASTLOG_ERROR("error: recv timed out\n");
		} else {
			MIRACASTLOG_ERROR("error: recv failed, or connection closed\n");
		}
	}
	MIRACASTLOG_INFO("received string(%d) - %s\n", recv_return, buffer);
	return status;
}

bool MiracastPrivate::initiateTCP(std::string goIP)
{
    int r, i, num_ready=0;
    size_t addr_size;
    struct epoll_event events[MAX_EPOLL_EVENTS];
    struct sockaddr_in addr = {0};
    struct sockaddr_storage str_addr = {0};
    std::string defaultgoIP = "192.168.49.1";

    system("iptables -I INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
    system("iptables -I OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(7236);
    
    if (goIP.empty()){
	    r = inet_pton(AF_INET, goIP.c_str(), &addr.sin_addr);
    }
    else
    {
	    r = inet_pton(AF_INET, goIP.c_str(), &addr.sin_addr);
    }
    
    if (r != 1)
    {
	    MIRACASTLOG_ERROR("inet_issue");
	    return false;
    }

    memcpy(&str_addr, &addr, sizeof(addr));
    addr_size = sizeof(addr);

    struct sockaddr_storage in_addr = str_addr;

    m_tcpSockfd = socket(in_addr.ss_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (m_tcpSockfd < 0)
    {
        MIRACASTLOG_ERROR("TCP Socket creation error %s", strerror(errno));
        return false;
    }

    /*---Add socket to epoll---*/
    int epfd = epoll_create(1);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = m_tcpSockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, m_tcpSockfd, &event);

#ifdef ENABLE_NON_BLOCKING
    fcntl(m_tcpSockfd, F_SETFL, O_NONBLOCK);
    MIRACASTLOG_INFO("NON_BLOCKING Socket Enabled...\n");
#endif

    r = connect(m_tcpSockfd, (struct sockaddr*)&in_addr, addr_size);
    if(r < 0)
    {
	    if(errno != EINPROGRESS)
	    {
		    MIRACASTLOG_INFO("Event %s received(%d)", strerror(errno), errno);
	    }
	    else
	    {
#ifdef ENABLE_NON_BLOCKING
		    // connection in progress
		    if (!waitDataTimeout( m_tcpSockfd , SOCKET_WAIT_TIMEOUT_IN_MILLISEC ))
		    {
			    // connection timed out or failed
			    MIRACASTLOG_INFO("Socket Connection Timedout ...\n");
		    }
		    else
		    {
			    // connection successful
			    // do something with the connected socket
			    MIRACASTLOG_INFO("Socket Connected Successfully ...\n");
		    }
	    }
#else
	    // connection timed out or failed
	    MIRACASTLOG_INFO("Event (%s) received", strerror(errno));
#endif
    }
    else
    {
	    MIRACASTLOG_INFO("Socket Connected Successfully ...\n");
    }

    /*---Wait for socket connect to complete---*/
    num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000/*timeout*/);
    for(i = 0; i < num_ready; i++)
    {
        if(events[i].events & EPOLLOUT)
        {
            MIRACASTLOG_INFO("Socket(%d) %d connected (EPOLLOUT)", i, events[i].data.fd);
        }
    }

    num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000/*timeout*/);
    for(i = 0; i < num_ready; i++)
    {
        if(events[i].events & EPOLLOUT)
        {
            MIRACASTLOG_INFO("Socket %d got some data via EPOLLOUT", events[i].data.fd);
            return true;
        }
    }
    return true; 
}

SESSION_MANAGER_ACTIONS MiracastPrivate::convertP2PtoSessionActions( enum P2P_EVENTS eventId )
{
	SESSION_MANAGER_ACTIONS action_type = SESSION_MGR_INVALID_ACTION;

	switch(eventId)
	{
		case EVENT_FOUND:
		{
			action_type = SESSION_MGR_GO_DEVICE_FOUND;
		}
		break;
		case EVENT_PROVISION:
		{
			action_type = SESSION_MGR_GO_DEVICE_PROVISION;
		}
		break;
		case EVENT_STOP:
		{
			action_type = SESSION_MGR_GO_STOP_FIND;
		}
		break;
		case EVENT_GO_NEG_REQ:
		{
			action_type = SESSION_MGR_GO_NEG_REQUEST;
		}
		break;
		case EVENT_GO_NEG_SUCCESS:
		{
			action_type = SESSION_MGR_GO_NEG_SUCCESS;
		}
		break;
		case EVENT_GO_NEG_FAILURE:
		{
			action_type = SESSION_MGR_GO_NEG_FAILURE;
		}
		break;
		case EVENT_GROUP_STARTED:
		{
			action_type = SESSION_MGR_GO_GROUP_STARTED;
		}
		break;
		case EVENT_FORMATION_SUCCESS:
		{
			action_type = SESSION_MGR_GO_GROUP_FORMATION_SUCCESS;
		}
		break;
		case EVENT_FORMATION_FAILURE:
		{
			action_type = SESSION_MGR_GO_GROUP_FORMATION_FAILURE;
		}
		break;
		case EVENT_DEVICE_LOST:
		{
			action_type = SESSION_MGR_GO_DEVICE_LOST;
		}
		break;
		case EVENT_GROUP_REMOVED:
		{
			action_type = SESSION_MGR_GO_GROUP_REMOVED;
		}
		break;
		case EVENT_ERROR:
		{
			action_type = SESSION_MGR_GO_EVENT_ERROR;
		}
		break;
		default:
		{
			action_type = SESSION_MGR_GO_UNKNOWN_EVENT;
		}
		break;
	}
	return action_type;
}

void MiracastPrivate::RestartSession( void )
{
	MIRACASTLOG_INFO("Restarting the Session ...\n");
	StopSession();
	discoverDevices();
}

void MiracastPrivate::StopSession( void )
{
	MIRACASTLOG_INFO("Stopping the Session ...\n");
	stopDiscoverDevices();
	if ( m_groupInfo ){
		delete m_groupInfo;
		m_groupInfo = NULL;
	}
}

#if 1 
void MiracastPrivate::evtHandler(enum P2P_EVENTS eventId, void* data, size_t len)
{
	SessionMgrMsg msg_buffer = {0};
	std::string event_buffer;
	MIRACASTLOG_VERBOSE("event received");
	if(m_isIARMEnabled)
	{
		IARM_BUS_WiFiSrvMgr_P2P_EventData_t* EventData = (IARM_BUS_WiFiSrvMgr_P2P_EventData_t*)data;
		event_buffer = EventData->event_data;
	}
	else
	{
		event_buffer = (char*) data;
		free(data);
	}
	msg_buffer.action = convertP2PtoSessionActions( eventId );
	strcpy( msg_buffer.event_buffer , event_buffer.c_str());
	MIRACASTLOG_INFO("evtHandler to SessionMgr Action[%#04X] buffer:%s  ",msg_buffer.action, event_buffer.c_str());
	m_session_manager_thread->send_message( &msg_buffer , sizeof(msg_buffer));
	MIRACASTLOG_VERBOSE("event received : %d buffer:%s  ",eventId, event_buffer.c_str());
}
#else
void MiracastPrivate::evtHandler(enum P2P_EVENTS eventId, void* data, size_t len)
{
	std::string event_buffer;
	MIRACASTLOG_VERBOSE("event received");
	if(m_isIARMEnabled)
	{
		IARM_BUS_WiFiSrvMgr_P2P_EventData_t* EventData = (IARM_BUS_WiFiSrvMgr_P2P_EventData_t*)data;
		event_buffer = EventData->event_data;
	}
	else
	{
		event_buffer = (char*) data;
		free(data);
	}
	MIRACASTLOG_VERBOSE("event received : %d buffer:%s  ",eventId, event_buffer.c_str());

	switch(eventId)
	{
		case EVENT_FOUND:
		{
			MIRACASTLOG_INFO("Application received device found %s", event_buffer.c_str());
			std::string wfdSubElements;
			DeviceInfo* device = new DeviceInfo;
			device->deviceMAC = storeData(event_buffer.c_str(), "p2p_dev_addr"); 
			device->deviceType = storeData(event_buffer.c_str(), "pri_dev_type"); 
			device->modelName = storeData(event_buffer.c_str(), "name"); 
			wfdSubElements = storeData(event_buffer.c_str(), "wfd_dev_info"); 
			device->isCPSupported = ((strtol(wfdSubElements.c_str(), NULL, 16) >> 32) && 256);
			device->deviceRole = (DEVICEROLE)((strtol(wfdSubElements.c_str(), NULL, 16) >> 32) && 3); 
			MIRACASTLOG_VERBOSE("Device data parsed & stored successfully");
			m_eventCallback->onDeviceDiscovery(device); 
			MIRACASTLOG_VERBOSE("onDeviceDiscovery Callback initiated");
			m_deviceInfo.push_back(device);
			break;
		}
		case EVENT_PROVISION:
		{
			MIRACASTLOG_INFO("Application received event provision %s", event_buffer.c_str());
			m_authType = "pbc";
			std::string MAC = storeData(event_buffer.c_str(), "p2p_dev_addr");
			m_eventCallback->onProvisionReq(m_authType, MAC);
			break;
		}
		case EVENT_STOP:
		{
			MIRACASTLOG_INFO("Stopping P2P find");
			break;
		}
		case EVENT_GO_NEG_REQ:
		{
			MIRACASTLOG_INFO("Handler received GoNegReq");
			std::string MAC;
			size_t space_find = event_buffer.find(" ");
			size_t dev_str = event_buffer.find("dev_passwd_id");
			if((space_find != std::string::npos) && (dev_str != std::string::npos))
			{
				MAC = event_buffer.substr(space_find, dev_str-space_find);
				REMOVE_SPACES(MAC);
				m_eventCallback->onGoNegReq(MAC);
			}
			break;
		}
		case EVENT_SHOW_PIN:
		{
			MIRACASTLOG_INFO("Application received event EVENT_SHOW_PIN %s", event_buffer.c_str());
			m_authType =event_buffer.substr(44,8);
			std::string MAC = storeData(event_buffer.c_str(), "p2p_dev_addr");
			MIRACASTLOG_INFO("Application received event MAC %s m_authType :%s  ", MAC.c_str(), m_authType.c_str());
			m_eventCallback->onProvisionReq(m_authType, MAC);
			break;
		}
		case EVENT_GO_NEG_SUCCESS:
		{
			break;
		}
		case EVENT_GO_NEG_FAILURE:
		{
			break;
		}
		case EVENT_GROUP_STARTED:
		{
			MIRACASTLOG_INFO("Group stated");
			m_groupInfo = new GroupInfo;
			int ret = -1;
			size_t found = event_buffer.find("client");
			size_t found_space = event_buffer.find(" ");
			if(found != std::string::npos)
			{
				m_groupInfo->ipAddr =  storeData(event_buffer.c_str(), "ip_addr");
				m_groupInfo->ipMask =  storeData(event_buffer.c_str(), "ip_mask");
				m_groupInfo->goIPAddr =  storeData(event_buffer.c_str(), "go_ip_addr");
				size_t found_client = event_buffer.find("client");
				m_groupInfo->interface =  event_buffer.substr(found_space, found_client-found_space);
				REMOVE_SPACES(m_groupInfo->interface);

				if(getenv("GET_PACKET_DUMP") != NULL)
				{
					std::string tcpdump;
					tcpdump.append("tcpdump -i ");
					tcpdump.append(m_groupInfo->interface);
					tcpdump.append(" -s 65535 -w /opt/dump.pcap &");
					MIRACASTLOG_VERBOSE("Dump command to execute - %s", tcpdump.c_str());
					system(tcpdump.c_str());
				}
				//STB is a client in the p2p group
				m_groupInfo->isGO = false;
				std::string localIP = startDHCPClient(m_groupInfo->interface);
				if(localIP.empty())
				{
					MIRACASTLOG_ERROR("Local IP address is not obtained");
					break;
				}
				else
				{
						ret = initiateTCP(m_groupInfo->goIPAddr);
				}
				if(ret == true)
				{
					ret = false;
					ret = connectSink();
					if(ret == true)
						MIRACASTLOG_INFO("Sink Connected successfully");
					else
					{
						MIRACASTLOG_FATAL("Sink Connection failure");
						return;
					}
				}
				else
				{
					MIRACASTLOG_FATAL("TCP connection Failed");
					return;
				}
			}
			else
			{
				size_t found_go = event_buffer.find("GO");
				m_groupInfo->interface =  event_buffer.substr(found_space, found_go-found_space);
				//STB is the GO in the p2p group
				m_groupInfo->isGO = true;
			}
			m_groupInfo->SSID =  storeData(event_buffer.c_str(), "ssid");
			m_groupInfo->goDevAddr =  storeData(event_buffer.c_str(), "go_dev_addr");
			m_connectionStatus = true;
			m_eventCallback->onGroupStarted();
			break;
		}
		case EVENT_FORMATION_SUCCESS:
		{
			MIRACASTLOG_INFO("Group formation successful");
			m_eventCallback->onGroupFormationSuccess();
			break;
		}
		case EVENT_FORMATION_FAILURE:
		{
			break;
		}
		case EVENT_DEVICE_LOST:
		{
			std::string lostMAC = storeData(event_buffer.c_str(), "p2p_dev_addr");
			size_t found;
			int i = 0;
			for(auto devices : m_deviceInfo)
			{
				found = devices->deviceMAC.find(lostMAC);
				if(found != std::string::npos) 
				{
					delete devices;
					m_deviceInfo.erase(m_deviceInfo.begin()+i);
					break;
				}
				i++;
			}
			m_eventCallback->onDeviceLost(lostMAC);
			break;
		}
		case EVENT_GROUP_REMOVED:
		{
			std::string reason = storeData(event_buffer.c_str(), "reason");
			m_eventCallback->onGroupRemoved(reason);
			break;
		}
		case EVENT_ERROR:
		{
			break;
		}
		default:
		break;
	}
}
#endif

MiracastError MiracastPrivate::discoverDevices()
{
	MiracastError ret = MIRACAST_FAIL;
	MIRACASTLOG_INFO("Discovering Devices");
	std::string command, retBuffer;

	command = "P2P_FIND";
	ret = executeCommand(command, NON_GLOBAL_INTERFACE, retBuffer);
	if(ret != MIRACAST_OK)
	{
		MIRACASTLOG_ERROR("Unable to discover devices");
	}
	return ret;
}

MiracastError MiracastPrivate::stopDiscoverDevices()
{
	MiracastError ret = MIRACAST_FAIL;
	MIRACASTLOG_INFO("Stop Discovering Devices");
	std::string command, retBuffer;

	command = "P2P_STOP_FIND";
	ret = executeCommand(command, NON_GLOBAL_INTERFACE, retBuffer);
	if(ret != MIRACAST_OK)
	{
		MIRACASTLOG_ERROR("Failed to Stop discovering devices");
	}
	return ret;
}


MiracastPrivate::MiracastPrivate(MiracastCallback* Callback)
{
	//To delete the rules so that it would be duplicated in this program execution
	system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
	system("iptables -D INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
	system("iptables -D OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
	p2p_init_done = false;
	m_tcpSockfd = -1;
	m_authType = "pbc";
	memset(event_buffer, '\0', sizeof(event_buffer));
	MIRACASTLOG_INFO("Private instance instantiated");

	g_miracastPrivate = this;

	m_client_req_handler_thread = new MiracastThread( CLIENT_REQ_HANDLER_NAME , CLIENT_REQ_HANDLER_STACK , 0 , 0 , reinterpret_cast<void (*)(void*)>(&ClientRequestHandlerCallback) , NULL );
	m_client_req_handler_thread->start();

	m_session_manager_thread = new MiracastThread( SESSION_MGR_NAME , SESSION_MGR_STACK , SESSION_MGR_MSG_COUNT , SESSION_MGR_MSGQ_SIZE , reinterpret_cast<void (*)(void*)>(&SessionMgrThreadCallback) , NULL );
        m_session_manager_thread->start();
	
	m_rtsp_msg_handler_thread = new MiracastThread( RTSP_HANDLER_NAME , RTSP_HANDLER_STACK , RTSP_HANDLER_MSG_COUNT , RTSP_HANDLER_MSGQ_SIZE , reinterpret_cast<void (*)(void*)>(&RTSPMsgHandlerCallback) , NULL );
	m_rtsp_msg_handler_thread->start();

	m_rtsp_msg = new MiracastRTSPMessages();

	wfdInit(Callback);
}

MiracastPrivate::~MiracastPrivate()
{
	MIRACASTLOG_INFO("Destructor...");
	if(m_isIARMEnabled)
	{
		p2pUninit();
	}

	delete m_rtsp_msg_handler_thread;
	delete m_rtsp_msg;
	delete m_session_manager_thread;
	delete m_client_req_handler_thread;

	while(!m_deviceInfo.empty())
	{
		delete m_deviceInfo.back();
		m_deviceInfo.pop_back();
	}

	if ( m_groupInfo ){
		delete m_groupInfo;
		m_groupInfo = NULL;
	}

	system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
	system("iptables -D INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
	system("iptables -D OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
}

MiracastError MiracastPrivate::connectDevice(std::string MAC)
{
	MIRACASTLOG_VERBOSE("Connecting to the MAC - %s", MAC.c_str());
	MiracastError ret = MIRACAST_FAIL;
	std::string command("P2P_CONNECT"), retBuffer;
	command.append(SPACE);
	command.append(MAC);
	command.append(SPACE);
	command.append(m_authType);

	ret = (MiracastError)executeCommand(command, NON_GLOBAL_INTERFACE, retBuffer);
	MIRACASTLOG_VERBOSE("Establishing P2P connection with authType - %s", m_authType.c_str());
	//m_eventCallback->onConnected();
	return ret;
}

MiracastError MiracastPrivate::startStreaming()
{
	const char* mcastIptableFile = "/opt/mcast_iptable.txt";
	std::ifstream mIpfile (mcastIptableFile);
	std::string mcast_iptable;
	if(mIpfile.is_open())
	{
		std::getline (mIpfile, mcast_iptable);
		MIRACASTLOG_INFO("Iptable reading from file [%s] as [ %s] ", mcastIptableFile , mcast_iptable.c_str());
		system(mcast_iptable.c_str());
		mIpfile.close();
	}
	else
	{
		system("iptables -I INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
	}
	MIRACASTLOG_INFO("Casting started. Player initiated");
	std::string gstreamerPipeline;

	const char* mcastfile = "/opt/mcastgstpipline.txt";
	std::ifstream mcgstfile (mcastfile);

	if ( mcgstfile.is_open() )
	{
		std::getline (mcgstfile, gstreamerPipeline);
		MIRACASTLOG_INFO("gstpipeline reading from file [%s], gstreamerPipeline as [ %s] ", mcastfile, gstreamerPipeline.c_str());
		mcgstfile.close();
		if (0 == system(gstreamerPipeline.c_str()))
			MIRACASTLOG_INFO("Pipeline created successfully ");
		else
		{
			MIRACASTLOG_INFO("Pipeline creation failure");
			return MIRACAST_FAIL;
		}
	}
	else
	{
		if(access( "/opt/miracast_gst", F_OK ) != 0)
		{
			gstreamerPipeline = "GST_DEBUG=3 gst-launch-1.0 -vvv udpsrc  port=1990 caps=\"application/x-rtp, media=video\" ! rtpmp2tdepay ! tsdemux name=demuxer demuxer. ! queue max-size-buffers=0 max-size-time=0 ! brcmvidfilter ! brcmvideodecoder ! brcmvideosink demuxer. ! queue max-size-buffers=0 max-size-time=0 ! brcmaudfilter ! brcmaudiodecoder ! brcmaudiosink";
			MIRACASTLOG_INFO("pipeline constructed is --> %s", gstreamerPipeline.c_str());
			if(0 == system(gstreamerPipeline.c_str()))
				MIRACASTLOG_INFO("Pipeline created successfully ");
			else
			{
				MIRACASTLOG_INFO("Pipeline creation failure");
				return MIRACAST_FAIL;
			}
		}
		else {
			//m_gstThread = new std::thread([]{ GStreamerThreadFunc(NULL); });
			GStreamerThreadFunc(NULL);
		}
	}

	// m_eventCallback->onStreamingStarted();
	return MIRACAST_OK;
}

bool MiracastPrivate::stopStreaming()
{
    system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
    return true;
}

bool MiracastPrivate::disconnectDevice()
{
    return true;
}

std::string MiracastPrivate::getConnectedMAC()
{
    return m_groupInfo->goDevAddr;
}

std::vector<DeviceInfo*> MiracastPrivate::getAllPeers()
{
    return m_deviceInfo;
}

bool MiracastPrivate::getConnectionStatus()
{
    return m_connectionStatus;
}

DeviceInfo* MiracastPrivate::getDeviceDetails(std::string MAC)
{
    DeviceInfo* deviceInfo = nullptr;
    std::size_t found;
    //memset(deviceInfo, 0, sizeof(deviceInfo));
    for(auto device : m_deviceInfo)
    {
        found = device->deviceMAC.find(MAC);
        if(found != std::string::npos)
        {
            deviceInfo = device;
            break;
        }
    }
    return deviceInfo;
}

bool MiracastPrivate::SendBufferTimedOut(std::string rtsp_response_buffer )
{
	int read_ret = 0;

	read_ret = send( m_tcpSockfd, rtsp_response_buffer.c_str(), rtsp_response_buffer.length(), 0 );

	if( 0 > read_ret )
	{
		MIRACASTLOG_INFO("Send Failed (%d)%s", errno, strerror(errno));
		return false;
	}

	MIRACASTLOG_INFO("Sending the RTSP response %d Data[%s]\n", read_ret,rtsp_response_buffer.c_str());
	return true;
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m1_msg_m2_send_request(std::string rtsp_m1_msg_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;
	size_t found = rtsp_m1_msg_buffer.find(RTSP_REQ_OPTIONS);

	MIRACASTLOG_INFO("M1 request received");
	if(found!=std::string::npos)
	{
		MIRACASTLOG_INFO("M1 OPTIONS packet received");
		std::stringstream ss(rtsp_m1_msg_buffer);
		std::string prefix = "";
		std::string req_str;
		std::string seq_str;
		std::string line;

		while (std::getline(ss, line)) {
			if (line.find(RTSP_STD_REQUIRE_FIELD) != std::string::npos) {
				prefix = RTSP_STD_REQUIRE_FIELD;
				req_str = line.substr(prefix.length());
				REMOVE_R(req_str);
				REMOVE_N(req_str);
			}
			else if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos) {
				prefix = RTSP_STD_SEQUENCE_FIELD;
				seq_str = line.substr(prefix.length());
				REMOVE_R(seq_str);
				REMOVE_N(seq_str);
			}
		}

		m_rtsp_msg->m1_msg_req_src2sink.append(rtsp_m1_msg_buffer);

		m_rtsp_msg->m1_msg_resp_sink2src.clear();
		m_rtsp_msg->m1_msg_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M1_RESPONSE ,  seq_str ,  req_str );

		MIRACASTLOG_INFO("Sending the M1 response \n-%s", m_rtsp_msg->m1_msg_resp_sink2src.c_str());

		if ( true == SendBufferTimedOut( m_rtsp_msg->m1_msg_resp_sink2src )){
			response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
			MIRACASTLOG_INFO("M1 response sent\n");

			m_rtsp_msg->m2_msg_req_sink2src.clear();
			m_rtsp_msg->m2_msg_req_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M2_REQUEST ,  dummy ,  req_str );

			MIRACASTLOG_INFO("%s", m_rtsp_msg->m2_msg_req_sink2src.c_str());
			MIRACASTLOG_INFO("Sending the M2 request \n");
			if ( true == SendBufferTimedOut( m_rtsp_msg->m2_msg_req_sink2src )){
				MIRACASTLOG_INFO("M2 request sent\n");
			}
			else{
				response_code = RTSP_SEND_REQ_RESPONSE_NOK;
				MIRACASTLOG_INFO("M2 request failed\n");
			}
		}
		else{
			response_code = RTSP_SEND_REQ_RESPONSE_NOK;
		}
	}
	return ( response_code );
}


RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m2_request_ack(std::string rtsp_m1_response_ack_buffer )
{
	//Yet to check and validate the response
	return RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m3_response_back(std::string rtsp_m3_msg_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;

	MIRACASTLOG_INFO("M3 request received");
	
	if ( rtsp_m3_msg_buffer.find("wfd_video_formats") != std::string::npos)
	{
		std::string seq_str = "";
		std::stringstream ss(rtsp_m3_msg_buffer);
		std::string prefix = "";
		std::string line;

		while (std::getline(ss, line)) {
			if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos) {
				prefix = RTSP_STD_SEQUENCE_FIELD;
				seq_str = line.substr(prefix.length());
				REMOVE_R(seq_str);
				REMOVE_N(seq_str);
				break;
			}
		}

		std::string content_buffer;
		m_rtsp_msg->m3_msg_req_src2sink.clear();
		m_rtsp_msg->m3_msg_req_src2sink.append(rtsp_m3_msg_buffer);

		m_rtsp_msg->m3_msg_resp_sink2src.clear();

		m_rtsp_msg->m3_msg_resp_sink2src.clear();
		m_rtsp_msg->m3_msg_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M3_RESPONSE ,  seq_str ,  dummy );

		MIRACASTLOG_INFO("%s", m_rtsp_msg->m3_msg_resp_sink2src.c_str());

		if ( true == SendBufferTimedOut( m_rtsp_msg->m3_msg_resp_sink2src )){
			response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
			MIRACASTLOG_INFO("Sending the M3 response \n");
		}
		else{
			response_code = RTSP_SEND_REQ_RESPONSE_NOK;
		}
	}

	return ( response_code );
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m4_response_back(std::string rtsp_m4_msg_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;

	if( rtsp_m4_msg_buffer.find("SET_PARAMETER") != std::string::npos)
	{
		std::string seq_str = "";
		std::string url = "";
		std::stringstream ss(rtsp_m4_msg_buffer);
		std::string prefix = "";
		std::string line;

		while (std::getline(ss, line)) {
			if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos) {
				prefix = RTSP_STD_SEQUENCE_FIELD;
				seq_str = line.substr(prefix.length());
				REMOVE_R(seq_str);
				REMOVE_N(seq_str);
			}
			else if (line.find(RTSP_STD_WFD_PRESENTATION_URL_FIELD) != std::string::npos) {
				prefix = RTSP_STD_WFD_PRESENTATION_URL_FIELD;
				std::size_t url_start_pos = line.find(prefix) + prefix.length();
				std::size_t url_end_pos = line.find( RTSP_SPACE_STR , url_start_pos);
				url = line.substr(url_start_pos, url_end_pos - url_start_pos);
				m_rtsp_msg->SetWFDPresentationURL(url);
			}
		}
		
		m_rtsp_msg->m4_msg_req_src2sink.clear();
		m_rtsp_msg->m4_msg_req_src2sink.append(rtsp_m4_msg_buffer);

		m_rtsp_msg->m4_msg_resp_sink2src.clear();
		m_rtsp_msg->m4_msg_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M4_RESPONSE ,  seq_str ,  dummy );

		MIRACASTLOG_INFO("Sending the M4 response \n");
		if ( true == SendBufferTimedOut( m_rtsp_msg->m4_msg_resp_sink2src )){
			response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
			MIRACASTLOG_INFO("M4 response sent\n");
		}
		else{
			response_code = RTSP_SEND_REQ_RESPONSE_NOK;
			MIRACASTLOG_INFO("Failed to sent M4 response\n");
		}
	}

	return ( response_code );
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m5_msg_m6_send_request(std::string rtsp_m5_msg_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;

	if( rtsp_m5_msg_buffer.find("wfd_trigger_method: SETUP") != std::string::npos)
	{
		std::string seq_str = "";
		std::stringstream ss(rtsp_m5_msg_buffer);
		std::string prefix = "";
		std::string line;

		while (std::getline(ss, line)) {
			if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos) {
				prefix = RTSP_STD_SEQUENCE_FIELD;
				seq_str = line.substr(prefix.length());
				REMOVE_R(seq_str);
				REMOVE_N(seq_str);
				break;
			}
		}

		m_rtsp_msg->m5_msg_req_src2sink.clear();
		m_rtsp_msg->m5_msg_req_src2sink.append(rtsp_m5_msg_buffer);

		m_rtsp_msg->m5_msg_resp_sink2src.clear();
		m_rtsp_msg->m5_msg_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M5_RESPONSE ,  seq_str ,  dummy );

		MIRACASTLOG_INFO("Sending the M5 response \n");
		if ( true == SendBufferTimedOut( m_rtsp_msg->m5_msg_resp_sink2src )){
			MIRACASTLOG_INFO("M5 Response has sent\n");

			m_rtsp_msg->m6_msg_req_sink2src.clear();
			m_rtsp_msg->m6_msg_req_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M6_REQUEST ,  seq_str ,  dummy );

			MIRACASTLOG_INFO("Sending the M6 Request\n");
			if ( true == SendBufferTimedOut( m_rtsp_msg->m6_msg_req_sink2src )){
				response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
				MIRACASTLOG_INFO("M6 Request has sent\n");
			}
			else{
				response_code = RTSP_SEND_REQ_RESPONSE_NOK;
				MIRACASTLOG_ERROR("Failed to Send the M6 Request\n");
			}
		}
		else{
			response_code = RTSP_SEND_REQ_RESPONSE_NOK;
			MIRACASTLOG_ERROR("Failed to Send the M5 response\n");
		}
	}
	return ( response_code );
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m6_ack_m7_send_request(std::string rtsp_m6_ack_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;

	if( !rtsp_m6_ack_buffer.empty())
	{
		m_rtsp_msg->m6_msg_req_ack_src2sink.clear();
		m_rtsp_msg->m6_msg_req_ack_src2sink.append(rtsp_m6_ack_buffer);

		size_t pos_ses = rtsp_m6_ack_buffer.find(RTSP_STD_SESSION_FIELD);
		std::string session = rtsp_m6_ack_buffer.substr(pos_ses+strlen(RTSP_STD_SESSION_FIELD));
		pos_ses = session.find(";");
		std::string session_number = session.substr(0, pos_ses);

		m_rtsp_msg->SetCurrentWFDSessionNumber( session_number );

		if(rtsp_m6_ack_buffer.find("client_port") != std::string::npos)
		{
			m_rtsp_msg->m7_msg_req_sink2src.clear();
			m_rtsp_msg->m7_msg_req_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M7_REQUEST ,  dummy ,  dummy );

			MIRACASTLOG_INFO("Sending the M7 Request\n");
			if ( true == SendBufferTimedOut( m_rtsp_msg->m7_msg_req_sink2src )){
				response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
				MIRACASTLOG_INFO("M7 Request has sent\n");
			}
			else{
				response_code = RTSP_SEND_REQ_RESPONSE_NOK;
				MIRACASTLOG_ERROR("Failed to Send the M7 Request\n");
			}
		}
	}

	return ( response_code );
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_m7_request_ack(std::string rtsp_m7_ack_buffer )
{
	// Yet to check and validate the M7 acknowledgement
	return RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_post_m1_m7_xchange(std::string rtsp_post_m1_m7_xchange_buffer )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;
	RTSP_SEND_RESPONSE_CODE sub_response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
	std::string rtsp_resp_sink2src = "";
	std::string seq_str = "";
	std::stringstream ss(rtsp_post_m1_m7_xchange_buffer);
	std::string prefix = "";
	std::string line;

	while (std::getline(ss, line)) {
		if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos) {
			prefix = RTSP_STD_SEQUENCE_FIELD;
			seq_str = line.substr(prefix.length());
			REMOVE_R(seq_str);
			REMOVE_N(seq_str);
			break;
		}
	}

	if( rtsp_post_m1_m7_xchange_buffer.find(RTSP_M16_REQUEST_MSG) != std::string::npos)
	{
		rtsp_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_M16_RESPONSE ,  seq_str ,  dummy );
	}
	else if( rtsp_post_m1_m7_xchange_buffer.find("wfd_trigger_method: TEARDOWN") != std::string::npos)
	{
		MIRACASTLOG_INFO("TEARDOWN request from Source received\n");
		rtsp_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_TEARDOWN_RESPONSE ,  seq_str ,  dummy );
		sub_response_code = RTSP_SRC_TEARDOWN_REQUEST;
	}

	if(!(rtsp_resp_sink2src.empty())){
		MIRACASTLOG_INFO("Sending the Response \n");
		if ( true == SendBufferTimedOut( rtsp_resp_sink2src )){
			response_code = sub_response_code;
			MIRACASTLOG_INFO("Response sent\n");
		}
		else{
			response_code = RTSP_SEND_REQ_RESPONSE_NOK;
			MIRACASTLOG_INFO("Failed to sent Response\n");
		}
	}

	return response_code;
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::handle_rtsp_msg_play_pause( RTSP_MSG_HANDLER_ACTIONS action_id )
{
	RTSP_MSG_FMT_SINK2SRC play_or_pause_mode = RTSP_MSG_FMT_INVALID;
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;

	if ( RTSP_PLAY_FROM_SINK2SRC == action_id ){
		play_or_pause_mode = RTSP_MSG_FMT_PLAY_REQUEST;
	}
	else if ( RTSP_PAUSE_FROM_SINK2SRC == action_id ){
		play_or_pause_mode = RTSP_MSG_FMT_PAUSE_REQUEST;
	}

	if ( RTSP_MSG_FMT_INVALID != play_or_pause_mode ){
		std::string rtsp_resp_sink2src;
		rtsp_resp_sink2src = m_rtsp_msg->GenerateRequestResponseFormat( RTSP_MSG_FMT_TEARDOWN_RESPONSE ,  dummy ,  dummy );

		if(!(rtsp_resp_sink2src.empty())){
			MIRACASTLOG_INFO("Sending the PLAY/PAUSE REQUEST \n");
			if ( true == SendBufferTimedOut( rtsp_resp_sink2src )){
				response_code = RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK;
				MIRACASTLOG_INFO("PLAY/PAUSE sent\n");
			}
			else{
				response_code = RTSP_SEND_REQ_RESPONSE_NOK;
				MIRACASTLOG_INFO("Failed to sent PLAY/PAUSE\n");
			}
		}
	}

	return response_code;
}

RTSP_SEND_RESPONSE_CODE MiracastPrivate::validate_rtsp_msg_response_back(std::string rtsp_msg_buffer , RTSP_MSG_HANDLER_ACTIONS action_id )
{
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_INVALID_MSG_RECEIVED;

	switch ( action_id )
	{
		case RTSP_M1_REQUEST_RECEIVED:
		{
			response_code = validate_rtsp_m1_msg_m2_send_request( rtsp_msg_buffer );
		}
		break;
		case RTSP_M2_REQUEST_ACK:
		{
			response_code = validate_rtsp_m2_request_ack( rtsp_msg_buffer );
		}
		break;
		case RTSP_M3_REQUEST_RECEIVED:
		{
			response_code = validate_rtsp_m3_response_back( rtsp_msg_buffer );
		}
		break;
		case RTSP_M4_REQUEST_RECEIVED:
		{
			response_code = validate_rtsp_m4_response_back( rtsp_msg_buffer );
		}
		break;
		case RTSP_M5_REQUEST_RECEIVED:
		{
			response_code = validate_rtsp_m5_msg_m6_send_request( rtsp_msg_buffer );
		}
		break;
		case RTSP_M6_REQUEST_ACK:
		{
			response_code = validate_rtsp_m6_ack_m7_send_request( rtsp_msg_buffer );
		}
		break;
		case RTSP_M7_REQUEST_ACK:
		{
			response_code = validate_rtsp_m7_request_ack( rtsp_msg_buffer );
		}
		break;
		case RTSP_MSG_POST_M1_M7_XCHANGE:
		{
			response_code = validate_rtsp_post_m1_m7_xchange( rtsp_msg_buffer );
		}
		break;
		default:
		{
			//
		}
	}
	MIRACASTLOG_INFO("Validating RTSP Msg => ACTION[%#04X] Resp[%#04X]\n",action_id,response_code);

	return response_code;
}

MiracastRTSPMessages::MiracastRTSPMessages()
{
	std::string default_configuration;

	m_current_sequence_number.clear();

	SetWFDEnableDisableUnicast(true);

	default_configuration = RTSP_DFLT_VIDEO_FORMATS;
	SetWFDVideoFormat( default_configuration );

	default_configuration = RTSP_DFLT_AUDIO_FORMATS;
	SetWFDAudioCodecs( default_configuration );

	default_configuration = RTSP_DFLT_CONTENT_PROTECTION;
	SetWFDContentProtection( default_configuration );

	default_configuration = RTSP_DFLT_TRANSPORT_PROFILE;
	SetWFDTransportProfile( default_configuration );

	default_configuration = RTSP_DFLT_STREAMING_PORT;
	SetWFDStreamingPortNumber( default_configuration );

	default_configuration = RTSP_DFLT_CLIENT_RTP_PORTS;
	SetWFDClientRTPPorts( default_configuration );
}

MiracastRTSPMessages::~MiracastRTSPMessages()
{
}

std::string MiracastRTSPMessages::GetRequestSequenceNumber(void)
{
	int next_number = std::stoi(m_current_sequence_number.empty() ? "0" : m_current_sequence_number) + 1;
	m_current_sequence_number = std::to_string(next_number);
	return m_current_sequence_number;
}

const char* MiracastRTSPMessages::GetRequestResponseFormat(RTSP_MSG_FMT_SINK2SRC format_type)
{
    int index = static_cast<RTSP_MSG_FMT_SINK2SRC>(format_type) - static_cast<RTSP_MSG_FMT_SINK2SRC>(RTSP_MSG_FMT_M1_RESPONSE);
    if (index >= 0 && index < static_cast<int>(sizeof(rtsp_msg_template_info)/sizeof(rtsp_msg_template_info[0]))) {
        return rtsp_msg_template_info[index].template_name;
    }
    return "";
}

std::string MiracastRTSPMessages::GenerateRequestResponseFormat( RTSP_MSG_FMT_SINK2SRC msg_fmt_needed , std::string received_session_no, std::string append_data1 )
{
	std::vector<const char*> sprintf_args;
	const char* template_str = GetRequestResponseFormat(msg_fmt_needed);
	std::string content_buffer = "";
	std::string unicast_supported = "";
	std::string content_buffer_len;
	std::string sequence_number = GetRequestSequenceNumber();
	std::string URL = GetWFDPresentationURL();
	std::string TSProfile = GetWFDTransportProfile();
	std::string StreamingPort = GetWFDStreamingPortNumber();
	std::string WFDSessionNum = GetCurrentWFDSessionNumber();

	// Determine the required buffer size using snprintf
	switch( msg_fmt_needed  ){
		case RTSP_MSG_FMT_M1_RESPONSE:
		{
			sprintf_args.push_back(append_data1.c_str());
			sprintf_args.push_back(received_session_no.c_str());
		}
		break;
		case RTSP_MSG_FMT_M3_RESPONSE:
		{
			// prepare content buffer
			// Append content protection type
			content_buffer.append(RTSP_STD_WFD_CONTENT_PROTECT_FIELD);
			content_buffer.append(GetWFDContentProtection());
			content_buffer.append(RTSP_CRLF_STR);
			// Append Video Formats
			content_buffer.append(RTSP_STD_WFD_VIDEO_FMT_FIELD);
			content_buffer.append(GetWFDVideoFormat());
			content_buffer.append(RTSP_CRLF_STR);
			// Append Audio Formats
			content_buffer.append(RTSP_STD_WFD_AUDIO_FMT_FIELD);
			content_buffer.append(GetWFDAudioCodecs());
			content_buffer.append(RTSP_CRLF_STR);
			// Append Client RTP Client port configuration
			content_buffer.append(RTSP_STD_WFD_CLIENT_PORTS_FIELD);
			content_buffer.append(GetWFDClientRTPPorts());
			content_buffer.append(RTSP_CRLF_STR);

			content_buffer_len = std::to_string(content_buffer.length());

			sprintf_args.push_back(content_buffer_len.c_str());
			sprintf_args.push_back(received_session_no.c_str());
			sprintf_args.push_back(content_buffer.c_str());

			MIRACASTLOG_INFO("content_buffer - [%s]\n",content_buffer.c_str());
		}
		break;
		case RTSP_MSG_FMT_M4_RESPONSE:
		case RTSP_MSG_FMT_M5_RESPONSE:
		case RTSP_MSG_FMT_M16_RESPONSE:
		case RTSP_MSG_FMT_TEARDOWN_RESPONSE:
		{
			sprintf_args.push_back(received_session_no.c_str());
		}
		break;
		case RTSP_MSG_FMT_M2_REQUEST:
		case RTSP_MSG_FMT_M6_REQUEST:
		case RTSP_MSG_FMT_M7_REQUEST:
		case RTSP_MSG_FMT_PAUSE_REQUEST:
		case RTSP_MSG_FMT_PLAY_REQUEST:
		case RTSP_MSG_FMT_TEARDOWN_REQUEST:
		{
			if ( RTSP_MSG_FMT_M2_REQUEST == msg_fmt_needed ){
				sprintf_args.push_back(append_data1.c_str());
			}
			else{
				sprintf_args.push_back(URL.c_str());

				if ( RTSP_MSG_FMT_M6_REQUEST == msg_fmt_needed ){
					sprintf_args.push_back(TSProfile.c_str());
					if ( true == IsWFDUnicastSupported()){
						unicast_supported.append(RTSP_STD_UNICAST_FIELD);
						unicast_supported.append(RTSP_SEMI_COLON_STR);
						sprintf_args.push_back(unicast_supported.c_str());
					}
					sprintf_args.push_back(StreamingPort.c_str());
				}
				else{
					sprintf_args.push_back(WFDSessionNum.c_str());
				}
			}
			sprintf_args.push_back(sequence_number.c_str());
		}
		break;
		default:
		{
			MIRACASTLOG_ERROR("INVALID FMT REQUEST\n");
		}
		break;
	}

	std::string result = "";

	if ( 0 != sprintf_args.size()){
		result = MiracastRTSPMessages::format_string( template_str , sprintf_args );
	}

    return result;
}

std::string MiracastRTSPMessages::GetWFDVideoFormat( void )
{
	return wfd_video_formats;
}

std::string MiracastRTSPMessages::GetWFDAudioCodecs( void )
{
	return wfd_audio_codecs;
}
std::string MiracastRTSPMessages::GetWFDClientRTPPorts( void )
{
	return wfd_client_rtp_ports;
}

std::string MiracastRTSPMessages::GetWFDUIBCCapability( void )
{
	return wfd_uibc_capability;
}

std::string MiracastRTSPMessages::GetWFDContentProtection( void )
{
	return wfd_content_protection;
}

std::string MiracastRTSPMessages::GetWFDSecScreenSharing( void )
{
	return wfd_sec_screensharing;
}

std::string MiracastRTSPMessages::GetWFDPortraitDisplay(void)
{
	return wfd_sec_portrait_display;
}

std::string MiracastRTSPMessages::GetWFDSecRotation( void )
{
	return wfd_sec_rotation;
}

std::string MiracastRTSPMessages::GetWFDSecHWRotation( void )
{
	return wfd_sec_hw_rotation;
}
std::string MiracastRTSPMessages::GetWFDSecFrameRate( void )
{
	return wfd_sec_framerate;
}

std::string MiracastRTSPMessages::GetWFDPresentationURL( void )
{
	return wfd_presentation_URL;
}

std::string MiracastRTSPMessages::GetWFDTransportProfile( void )
{
	return wfd_transport_profile;
}

std::string MiracastRTSPMessages::GetWFDStreamingPortNumber( void )
{
	return wfd_streaming_port;
}

bool MiracastRTSPMessages::IsWFDUnicastSupported( void )
{
	return is_unicast;
}

std::string MiracastRTSPMessages::GetCurrentWFDSessionNumber( void )
{
	return wfd_session_number;
}

bool MiracastRTSPMessages::SetWFDVideoFormat( std::string video_formats )
{
	wfd_video_formats = video_formats;
	return true;
}
bool MiracastRTSPMessages::SetWFDAudioCodecs( std::string audio_codecs )
{
	wfd_audio_codecs = audio_codecs;
	return true;
}
bool MiracastRTSPMessages::SetWFDClientRTPPorts( std::string client_rtp_ports )
{
	wfd_client_rtp_ports = client_rtp_ports;
	return true;
}
bool MiracastRTSPMessages::SetWFDUIBCCapability( std::string uibc_caps )
{
	wfd_uibc_capability = uibc_caps;
	return true;
}
bool MiracastRTSPMessages::SetWFDContentProtection( std::string content_protection )
{
	wfd_content_protection = content_protection;
	return true;
}
bool MiracastRTSPMessages::SetWFDSecScreenSharing( std::string screen_sharing )
{
	wfd_sec_screensharing = screen_sharing;
	return true;
}

bool MiracastRTSPMessages::SetWFDPortraitDisplay( std::string portrait_display )
{
	wfd_sec_portrait_display = portrait_display;
	return true;
}

bool MiracastRTSPMessages::SetWFDSecRotation( std::string rotation )
{
	wfd_sec_rotation = rotation;
	return true;
}
bool MiracastRTSPMessages::SetWFDSecHWRotation( std::string hw_rotation )
{
	wfd_sec_hw_rotation = hw_rotation;
	return true;
}
bool MiracastRTSPMessages::SetWFDSecFrameRate( std::string framerate )
{
	wfd_sec_framerate = framerate;
	return true;
}

bool MiracastRTSPMessages::SetWFDPresentationURL( std::string URL )
{
	wfd_presentation_URL = URL;
	return true;
}

bool MiracastRTSPMessages::SetWFDTransportProfile( std::string transport_profile )
{
	wfd_transport_profile = transport_profile;
	return true;
}

bool MiracastRTSPMessages::SetWFDStreamingPortNumber( std::string port_number )
{
	wfd_streaming_port = port_number;
	return true;
}

bool MiracastRTSPMessages::SetWFDEnableDisableUnicast( bool enable_disable_unicast )
{
	is_unicast = enable_disable_unicast;
	return true;
}

bool MiracastRTSPMessages::SetCurrentWFDSessionNumber( std::string session )
{
	wfd_session_number = session;
	return true;
}

MiracastThread::MiracastThread(std::string thread_name, size_t stack_size, size_t msg_size, size_t queue_depth , void (*callback)(void*) , void* user_data )
{
	thread_name = thread_name;
	
	thread_stacksize = stack_size;
	thread_message_size = msg_size;
	thread_message_count = queue_depth;

	thread_user_data = user_data;

	thread_callback = callback;

	// Create message queue
	g_queue = g_async_queue_new();
	//g_async_queue_ref( g_queue );
	
	sem_init( &sem_object , 0 , 0 );
	
	// Create thread
	pthread_attr_init(&attr_);
	pthread_attr_setstacksize(&attr_, stack_size); 
}

MiracastThread::~MiracastThread()
{
	// Join thread
	pthread_join(thread_, NULL);

	// Close message queue
	g_async_queue_unref( g_queue );	
}

void MiracastThread::start(void)
{
	pthread_create( &thread_, &attr_, reinterpret_cast<void* (*)(void*)>(thread_callback) , thread_user_data );
}

void MiracastThread::send_message( void* message , size_t msg_size )
{
	void* buffer = malloc( msg_size );
	if ( NULL == buffer ){
		MIRACASTLOG_ERROR("Memory Allocation Failed for %u\n",msg_size);
		return;
	}
	memset( buffer , 0x00 , msg_size );
	// Send message to queue

	memcpy( buffer , message , msg_size );
	g_async_queue_push( this->g_queue, buffer );
	sem_post( &this->sem_object );
}

int8_t MiracastThread::receive_message( void* message , size_t msg_size , int sem_wait_timedout )
{
	int8_t status = false;

	if ( MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT == sem_wait_timedout ){
		sem_wait( &this->sem_object );
		status = true;
	}
	else if ( 0 < sem_wait_timedout ){
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += sem_wait_timedout;

		if ( -1 != sem_timedwait(&sem_object, &ts))
		{
			status = true;
		}
	}
	else{
		status = -1;
	}

	if (( true == status ) && ( NULL != this->g_queue )){
		void* data_ptr = static_cast<void*>(g_async_queue_pop(this->g_queue));
		if (( NULL != message ) && ( NULL != data_ptr )){
			memcpy(message , data_ptr , msg_size );
			free(data_ptr);
		}	

	}
	return status;
}

void MiracastPrivate::SessionManagerThread(void* args)
{
	SessionMgrMsg session_message_data = { 0 };
	RTSPHldrMsg rtsp_message_data = { RTSP_INVALID_ACTION , 0 };
	bool client_req_client_connection_sent = false;

	while( true ){
		std::string event_buffer;
		event_buffer.clear();

		MIRACASTLOG_INFO("[%s] Waiting for Event .....\n",__FUNCTION__);
		m_session_manager_thread->receive_message( &session_message_data , SESSION_MGR_MSGQ_SIZE , MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT );
		
		event_buffer = session_message_data.event_buffer;

		MIRACASTLOG_INFO("[%s] Received Action[%#04X]Data[%s]\n",__FUNCTION__,session_message_data.action,event_buffer.c_str());

		if ( SESSION_MGR_SELF_ABORT == session_message_data.action ){
			MIRACASTLOG_INFO("SESSION_MGR_SELF_ABORT Received.\n");
			rtsp_message_data.action = RTSP_SELF_ABORT;
			m_rtsp_msg_handler_thread->send_message( &rtsp_message_data , RTSP_HANDLER_MSGQ_SIZE);
			StopSession();
			break;
		}

		switch ( session_message_data.action )
		{
			case SESSION_MGR_START_DISCOVERING:
			{
				MIRACASTLOG_INFO("SESSION_MGR_START_DISCOVERING Received\n");
				setWiFiDisplayParams();
				discoverDevices();
			}
			break;
			case SESSION_MGR_STOP_DISCOVERING:
			{
				MIRACASTLOG_INFO("SESSION_MGR_STOP_DISCOVERING Received\n");
				StopSession();
			}
			break;
			case SESSION_MGR_GO_DEVICE_FOUND:
			{
				MIRACASTLOG_INFO("SESSION_MGR_GO_DEVICE_FOUND Received\n");
				std::string wfdSubElements;
				DeviceInfo* device = new DeviceInfo;
				device->deviceMAC = storeData(event_buffer.c_str(), "p2p_dev_addr"); 
				device->deviceType = storeData(event_buffer.c_str(), "pri_dev_type"); 
				device->modelName = storeData(event_buffer.c_str(), "name"); 
				wfdSubElements = storeData(event_buffer.c_str(), "wfd_dev_info");
#if 0
				device->isCPSupported = ((strtol(wfdSubElements.c_str(), NULL, 16) >> 32) && 256);
				device->deviceRole = (DEVICEROLE)((strtol(wfdSubElements.c_str(), NULL, 16) >> 32) && 3); 
#endif
				MIRACASTLOG_VERBOSE("Device data parsed & stored successfully");
				
				m_deviceInfo.push_back(device);
			}
			break;
			case SESSION_MGR_GO_DEVICE_LOST:
			{
				MIRACASTLOG_INFO("SESSION_MGR_GO_DEVICE_LOST Received\n");
				std::string lostMAC = storeData(event_buffer.c_str(), "p2p_dev_addr");
				size_t found;
				int i = 0;
				for(auto devices : m_deviceInfo)
				{
					found = devices->deviceMAC.find(lostMAC);
					if(found != std::string::npos) 
					{
						delete devices;
						m_deviceInfo.erase(m_deviceInfo.begin()+i);
						break;
					}
					i++;
				}
			}
			break;
			case SESSION_MGR_GO_DEVICE_PROVISION:
			{
				MIRACASTLOG_INFO("SESSION_MGR_GO_DEVICE_PROVISION Received\n");
				m_authType = "pbc";
				std::string MAC = storeData(event_buffer.c_str(), "p2p_dev_addr");
			}
			break;
			case SESSION_MGR_GO_NEG_REQUEST:
			{
				ClientReqHldrMsg client_req_msg_data = {0};
				MIRACASTLOG_INFO("SESSION_MGR_GO_NEG_REQUEST Received\n");
				MIRACASTLOG_INFO("Handler received GoNegReq");
				std::string MAC;
				size_t space_find = event_buffer.find(" ");
				size_t dev_str = event_buffer.find("dev_passwd_id");
				if((space_find != std::string::npos) && (dev_str != std::string::npos))
				{
					MAC = event_buffer.substr(space_find, dev_str-space_find);
					REMOVE_SPACES(MAC);
				}

				size_t found;
				std::string device_name;
				int i = 0;
				for(auto devices : m_deviceInfo)
				{
					found = devices->deviceMAC.find(MAC);
					if(found != std::string::npos)
					{
						device_name = devices->modelName;
						break;
					}
					i++;
				}

				if ( false == client_req_client_connection_sent ){
					client_req_msg_data.action = CLIENT_REQ_HLDR_CONNECT_DEVICE_FROM_SESSION_MGR;
					strcpy( client_req_msg_data.action_buffer , MAC.c_str());
					strcpy( client_req_msg_data.buffer_user_data , device_name.c_str());
					m_client_req_handler_thread->send_message( &client_req_msg_data , sizeof(client_req_msg_data));
					client_req_client_connection_sent = true;
				}
			}
			break;
			case SESSION_MGR_CONNECT_REQ_FROM_HANDLER:
			{
				MIRACASTLOG_INFO("SESSION_MGR_CONNECT_REQ_FROM_HANDLER Received\n");
				std::string mac_address = event_buffer;

				connectDevice( mac_address );
				client_req_client_connection_sent = false;
			}
			break;
			case SESSION_MGR_GO_GROUP_STARTED:
			{
				MIRACASTLOG_INFO("SESSION_MGR_GO_GROUP_STARTED Received\n");
				m_groupInfo = new GroupInfo;
				int ret = -1;
				size_t found = event_buffer.find("client");
				size_t found_space = event_buffer.find(" ");
				if(found != std::string::npos)
				{
					m_groupInfo->ipAddr =  storeData(event_buffer.c_str(), "ip_addr");
					m_groupInfo->ipMask =  storeData(event_buffer.c_str(), "ip_mask");
					m_groupInfo->goIPAddr =  storeData(event_buffer.c_str(), "go_ip_addr");
					m_groupInfo->goDevAddr =  storeData(event_buffer.c_str(), "go_dev_addr");

					size_t found_client = event_buffer.find("client");
					m_groupInfo->interface =  event_buffer.substr(found_space, found_client-found_space);
					REMOVE_SPACES(m_groupInfo->interface);

					if(getenv("GET_PACKET_DUMP") != NULL)
					{
						std::string tcpdump;
						tcpdump.append("tcpdump -i ");
						tcpdump.append(m_groupInfo->interface);
						tcpdump.append(" -s 65535 -w /opt/dump.pcap &");
						MIRACASTLOG_VERBOSE("Dump command to execute - %s", tcpdump.c_str());
						system(tcpdump.c_str());
					}
					//STB is a client in the p2p group
					m_groupInfo->isGO = false;
					std::string localIP = startDHCPClient(m_groupInfo->interface);
					if(localIP.empty())
					{
						MIRACASTLOG_ERROR("Local IP address is not obtained");
						break;
					}
					else
					{
						MIRACASTLOG_VERBOSE("initiateTCP started GO IP[%s]\n",m_groupInfo->goIPAddr.c_str());
						ret = initiateTCP(m_groupInfo->goIPAddr);
						MIRACASTLOG_VERBOSE("initiateTCP done ret[%x]\n",ret);
					}

					if(ret == true)
					{
						rtsp_message_data.action = RTSP_START_RECEIVE_MSGS;
						MIRACASTLOG_INFO("RTSP Thread Initialated with RTSP_START_RECEIVE_MSGS\n");
						m_rtsp_msg_handler_thread->send_message( &rtsp_message_data , RTSP_HANDLER_MSGQ_SIZE);
						ret = false;
					}
					else
					{
						MIRACASTLOG_FATAL("TCP connection Failed");
						continue;
					}
				}
				else
				{
					size_t found_go = event_buffer.find("GO");
					m_groupInfo->interface =  event_buffer.substr(found_space, found_go-found_space);
					//STB is the GO in the p2p group
					m_groupInfo->isGO = true;
				}
				m_groupInfo->SSID =  storeData(event_buffer.c_str(), "ssid");
				m_connectionStatus = true;
			}
			break;
			case SESSION_MGR_GO_GROUP_REMOVED:
			{
				MIRACASTLOG_INFO("SESSION_MGR_GO_GROUP_REMOVED Received\n");
				std::string reason = storeData(event_buffer.c_str(), "reason");
				RestartSession();
			}
			break;
			case SESSION_MGR_START_STREAMING:
			{
				MIRACASTLOG_INFO("SESSION_MGR_START_STREAMING Received\n");
				startStreaming();
			}
			break;
			case SESSION_MGR_PAUSE_STREAMING:
			{
				MIRACASTLOG_INFO("SESSION_MGR_PAUSE_STREAMING Received\n");
			}
			break;
			case SESSION_MGR_STOP_STREAMING:
			{
				MIRACASTLOG_INFO("SESSION_MGR_STOP_STREAMING Received\n");
			}
			break;
			case SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY:
			{
				MIRACASTLOG_INFO("SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY Received\n");
				startStreaming();
			}
			break;
			case SESSION_MGR_RTSP_MSG_TIMEDOUT:
			case SESSION_MGR_RTSP_INVALID_MESSAGE:
			case SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED:
			case SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED:
			{
				MIRACASTLOG_INFO("[TIMEDOUT/TEARDOWN_REQ/SEND_REQ_RESP_FAIL/INVALID_MESSAG] Received\n");
				RestartSession();
			}
			break;
			case SESSION_MGR_CONNECT_REQ_REJECT_OR_TIMEOUT:
			{
				client_req_client_connection_sent = false;
			}
			break;
			case SESSION_MGR_GO_NEG_FAILURE:
			case SESSION_MGR_GO_GROUP_FORMATION_FAILURE:
			{
				MIRACASTLOG_INFO("[GO_NEG/GROUP_FORMATION_FAILURE] Received\n");
				RestartSession();
			}
			break;
			case SESSION_MGR_GO_STOP_FIND:
			case SESSION_MGR_GO_NEG_SUCCESS:
			case SESSION_MGR_GO_GROUP_FORMATION_SUCCESS:
			case SESSION_MGR_GO_EVENT_ERROR:
			case SESSION_MGR_GO_UNKNOWN_EVENT:
			case SESSION_MGR_SELF_ABORT:
			case SESSION_MGR_INVALID_ACTION:
			{
				MIRACASTLOG_INFO("[STOP_FIND/NEG_SUCCESS/GROUP_FORMATION_SUCCESS/EVENT_ERROR] Received\n");
			}
			break;
		}
	}
}

void MiracastPrivate::RTSPMessageHandlerThread( void* args )
{
	char rtsp_message_socket[4096] = {0};
	RTSPHldrMsg message_data = {};
	SessionMgrMsg session_mgr_buffer = {0};
	RTSP_SEND_RESPONSE_CODE response_code = RTSP_RECV_TIMEDOUT;
	std::string socket_buffer;
	bool start_monitor_keep_alive_msg = false;

	while( true ){
		MIRACASTLOG_INFO("[%s] Waiting for Event .....\n",__FUNCTION__);
		m_rtsp_msg_handler_thread->receive_message( &message_data , sizeof(message_data) , MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT );

		MIRACASTLOG_INFO("[%s] Received Action[%#04X]\n",__FUNCTION__,message_data.action);

		if ( RTSP_SELF_ABORT == message_data.action ){
			MIRACASTLOG_INFO("RTSP_SELF_ABORT ACTION Received\n");
			break;
		}

		if ( RTSP_START_RECEIVE_MSGS != message_data.action ){
			continue;
		}
		message_data.action = RTSP_M1_REQUEST_RECEIVED;

		memset( &rtsp_message_socket , 0x00 , sizeof(rtsp_message_socket));

		while( ReceiveBufferTimedOut( rtsp_message_socket , sizeof(rtsp_message_socket)))
		{
			socket_buffer.clear();
			socket_buffer = rtsp_message_socket;

			response_code = validate_rtsp_msg_response_back( socket_buffer , message_data.action );
		
			MIRACASTLOG_INFO("[%s] Validate RTSP Msg Action[%#04X] Response[%#04X]\n",__FUNCTION__,message_data.action,response_code);

			if (( RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK != response_code) || ( RTSP_M7_REQUEST_ACK == message_data.action )){
				break;
			}
			memset( &rtsp_message_socket , 0x00 , sizeof(rtsp_message_socket));
			message_data.action = static_cast<RTSP_MSG_HANDLER_ACTIONS>(message_data.action + 1);
		}

		start_monitor_keep_alive_msg = false;

		if (( RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK == response_code ) && ( RTSP_M7_REQUEST_ACK == message_data.action ))
		{
			session_mgr_buffer.action = SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY;
			start_monitor_keep_alive_msg = true;
		}
		else if ( RTSP_INVALID_MSG_RECEIVED == response_code ){
			session_mgr_buffer.action = SESSION_MGR_RTSP_INVALID_MESSAGE;
		}
		else if ( RTSP_SEND_REQ_RESPONSE_NOK == response_code ){
			session_mgr_buffer.action = SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED;
		}
		else{
			session_mgr_buffer.action = SESSION_MGR_RTSP_MSG_TIMEDOUT;
		}
		MIRACASTLOG_INFO("Msg to SessionMgr Action[%#04X]\n",session_mgr_buffer.action);
		m_session_manager_thread->send_message( &session_mgr_buffer , sizeof(session_mgr_buffer));

		while( true == start_monitor_keep_alive_msg )
		{
			if ( ReceiveBufferTimedOut( rtsp_message_socket , sizeof(rtsp_message_socket))){
				socket_buffer.clear();
				socket_buffer = rtsp_message_socket;
				MIRACASTLOG_INFO("\n #### RTSP Message [%s] #### \n",socket_buffer.c_str());

				response_code = validate_rtsp_msg_response_back( socket_buffer , RTSP_MSG_POST_M1_M7_XCHANGE );

				MIRACASTLOG_INFO("[%s] Validate RTSP Msg Action[%#04X] Response[%#04X]\n",__FUNCTION__,message_data.action,response_code);
				if (( RTSP_SRC_TEARDOWN_REQUEST == response_code)||
					(( RTSP_VALID_MSG_OR_SEND_REQ_RESPONSE_OK != response_code)&&
					( RTSP_SRC_TEARDOWN_REQUEST != response_code))){
					session_mgr_buffer.action = SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED;
					MIRACASTLOG_INFO("Msg to SessionMgr Action[%#04X]\n",session_mgr_buffer.action);
					m_session_manager_thread->send_message( &session_mgr_buffer , sizeof(session_mgr_buffer));
					break;
				}
				memset( &rtsp_message_socket , 0x00 , sizeof(rtsp_message_socket));
			}

			MIRACASTLOG_INFO("[%s] Waiting for Event .....\n",__FUNCTION__);
			if ( true == m_rtsp_msg_handler_thread->receive_message( &message_data , sizeof(message_data) , 1 )){
				MIRACASTLOG_INFO("[%s] Received Action[%#04X]\n",__FUNCTION__,message_data.action);
				if (( RTSP_SELF_ABORT == message_data.action )||( RTSP_RESTART == message_data.action )){
					MIRACASTLOG_INFO("RTSP_SELF_ABORT/RTSP_RESTART ACTION Received\n");
					break;
				}
				else if (( RTSP_PLAY_FROM_SINK2SRC == message_data.action )||( RTSP_PAUSE_FROM_SINK2SRC == message_data.action )){
					handle_rtsp_msg_play_pause( message_data.action );
				}
			}
		}

		MIRACASTLOG_INFO("[%s] Received Action[%#04X]\n",__FUNCTION__,message_data.action);
		if ( RTSP_SELF_ABORT == message_data.action ){
			MIRACASTLOG_INFO("RTSP_SELF_ABORT ACTION Received\n");
			break;
		}
	}
}

void MiracastPrivate::ClientRequestHandlerThread( void* args )
{
	SessionMgrMsg session_mgr_msg_data = {0};
	ClientReqHldrMsg client_req_hldr_msg_data = {0};
	bool send_message = false;

	while( true )
	{
		send_message = true;
		memset( &session_mgr_msg_data , 0x00 , SESSION_MGR_MSGQ_SIZE );

		MIRACASTLOG_INFO("[%s] Waiting for Event .....\n",__FUNCTION__);
		m_client_req_handler_thread->receive_message( &client_req_hldr_msg_data , sizeof(client_req_hldr_msg_data) , MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT );

		MIRACASTLOG_INFO("[%s] Received Action[%#04X]\n",__FUNCTION__,client_req_hldr_msg_data.action);

		switch (client_req_hldr_msg_data.action)
		{
			case CLIENT_REQ_HLDR_START_DISCOVER:
			{
				session_mgr_msg_data.action = SESSION_MGR_START_DISCOVERING;
				MIRACASTLOG_INFO("App: Started Device discovery\n");
			}	
			break;
			case CLIENT_REQ_HLDR_STOP_DISCOVER:
			{
				session_mgr_msg_data.action = SESSION_MGR_STOP_DISCOVERING;
			}
			break;
			case CLIENT_REQ_HLDR_CONNECT_DEVICE_FROM_SESSION_MGR:
			{
				std::string device_name = client_req_hldr_msg_data.buffer_user_data;
				std::string MAC = client_req_hldr_msg_data.action_buffer;

				send_message = true;
				MIRACASTLOG_INFO("\n################# GO DEVICE[%s - %s] wants to connect: #################\n",device_name.c_str(),MAC.c_str());

				if ( true == m_client_req_handler_thread->receive_message( &client_req_hldr_msg_data , sizeof(client_req_hldr_msg_data) , CLIENT_REQ_THREAD_CLIENT_CONNECTION_WAITTIME )){
					MIRACASTLOG_INFO("ClientReqHandler Msg Received [%#04X]\n",client_req_hldr_msg_data.action);
					if ( CLIENT_REQ_HLDR_CONNECT_DEVICE_ACCEPTED == client_req_hldr_msg_data.action ){
						strcpy( session_mgr_msg_data.event_buffer , MAC.c_str());
						session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_FROM_HANDLER;
					}
					else if ( CLIENT_REQ_HLDR_CONNECT_DEVICE_REJECTED == client_req_hldr_msg_data.action ){
						session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_REJECT_OR_TIMEOUT;
					}
					else if ( CLIENT_REQ_HLDR_STOP_APPLICATION == client_req_hldr_msg_data.action ){
						session_mgr_msg_data.action = SESSION_MGR_SELF_ABORT;
					}
					else if ( CLIENT_REQ_HLDR_STOP_DISCOVER == client_req_hldr_msg_data.action ){
						 session_mgr_msg_data.action = SESSION_MGR_STOP_DISCOVERING;
					}
					else{
						session_mgr_msg_data.action = SESSION_MGR_INVALID_ACTION;
					}
				}
				else{
					session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_REJECT_OR_TIMEOUT;
				}
			}
			break;
			case CLIENT_REQ_HLDR_STOP_APPLICATION:
			{
				session_mgr_msg_data.action = SESSION_MGR_SELF_ABORT;
			}
			break;
			default:
			{
				//
			}
			break;
		}

		if ( true == send_message ){
			MIRACASTLOG_INFO("Msg to SessionMgr Action[%#04X]\n",session_mgr_msg_data.action);
			m_session_manager_thread->send_message( &session_mgr_msg_data , SESSION_MGR_MSGQ_SIZE );
			if ( CLIENT_REQ_HLDR_STOP_APPLICATION == client_req_hldr_msg_data.action ){
				break;
			}
		}
	}
}

void MiracastPrivate::SendMessageToClientReqHandler( size_t action )
{
	ClientReqHldrMsg client_message_data = {0};
	bool valid_mesage = true;

	switch ( action )
	{
		case Start_WiFi_Display:
		{
			client_message_data.action = CLIENT_REQ_HLDR_START_DISCOVER;
		}
		break;
		case Stop_WiFi_Display:
		{
			client_message_data.action = CLIENT_REQ_HLDR_STOP_DISCOVER;
		}
		break;
		case Stop_Miracast_Service:
		{
			client_message_data.action = CLIENT_REQ_HLDR_STOP_APPLICATION;
		}
		break;
		case Accept_ConnectDevice_Request:
		{
			client_message_data.action = CLIENT_REQ_HLDR_CONNECT_DEVICE_ACCEPTED; 
		}
		break;
		case Reject_ConnectDevice_Request:
		{
			client_message_data.action = CLIENT_REQ_HLDR_CONNECT_DEVICE_REJECTED;
		}
		break;
		default:
		{
			valid_mesage = false;
		}
		break;
	}
	
	MIRACASTLOG_VERBOSE("MiracastPrivate::SendMessageToClientReqHandler received Action[%#04X]\n",action);

	if ( true == valid_mesage ){
		MIRACASTLOG_VERBOSE("Msg to ClientReqHldr Action[%#04X]\n",client_message_data.action);
		m_client_req_handler_thread->send_message( &client_message_data , sizeof(client_message_data));
	}
}

void ClientRequestHandlerCallback( void* args )
{
	g_miracastPrivate->ClientRequestHandlerThread(NULL);
}

void SessionMgrThreadCallback( void* args )
{
	g_miracastPrivate->SessionManagerThread(NULL);
}

void RTSPMsgHandlerCallback( void* args )
{
	g_miracastPrivate->RTSPMessageHandlerThread(NULL);
}
