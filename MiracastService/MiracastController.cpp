/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#include "MiracastController.h"
#include "MiracastPlayer.h"

static std::string empty_string = "";

void PluginReqHandlerCallback(void *args);
void SessionMgrThreadCallback(void *args);
void RTSPMsgHandlerCallback(void *args);

#ifdef ENABLE_HDCP2X_SUPPORT
void HDCPHandlerCallback(void *args);
#endif

MiracastController *MiracastController::m_miracast_ctrl_obj{nullptr};

MiracastController *MiracastController::getInstance(MiracastServiceNotifier *notifier)
{
    MIRACASTLOG_TRACE("Entering...");
    if (nullptr == m_miracast_ctrl_obj)
    {
        m_miracast_ctrl_obj = new MiracastController();
        if (nullptr != m_miracast_ctrl_obj)
        {
            m_miracast_ctrl_obj->m_notify_handler = notifier;
            m_miracast_ctrl_obj->create_ControllerFramework();
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return m_miracast_ctrl_obj;
}

void MiracastController::destroyInstance()
{
    MIRACASTLOG_TRACE("Entering...");
    if (nullptr != m_miracast_ctrl_obj)
    {
        m_miracast_ctrl_obj->destroy_ControllerFramework();
        delete m_miracast_ctrl_obj;
        m_miracast_ctrl_obj = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting...");
}

MiracastController::MiracastController(void)
{
    MIRACASTLOG_TRACE("Entering...");
    // To delete the rules so that it would be duplicated in this program execution
    system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
    system("iptables -D INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
    system("iptables -D OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");

    m_tcpSockfd = -1;
    m_groupInfo = nullptr;
    MIRACASTLOG_TRACE("Exiting...");
}

MiracastController::~MiracastController()
{
    MIRACASTLOG_TRACE("Entering...");

    while (!m_deviceInfoList.empty())
    {
        delete m_deviceInfoList.back();
        m_deviceInfoList.pop_back();
    }

    if (nullptr != m_groupInfo)
    {
        delete m_groupInfo;
        m_groupInfo = nullptr;
    }

    /*@TODO: Check on ACCEPT or REJECT. (p2)*/
    system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
    system("iptables -D INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
    system("iptables -D OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastController::create_ControllerFramework(void)
{
    MIRACASTLOG_TRACE("Entering...");
    m_rtsp_msg = new MiracastRTSPMsg();

    m_plugin_req_handler_thread = new MiracastThread(PLUGIN_REQ_HANDLER_NAME, PLUGIN_REQ_HANDLER_STACK, 0, 0, reinterpret_cast<void (*)(void *)>(&PluginReqHandlerCallback), this);
    m_plugin_req_handler_thread->start();

    m_session_manager_thread = new MiracastThread(SESSION_MGR_NAME, SESSION_MGR_STACK, SESSION_MGR_MSG_COUNT, SESSION_MGR_MSGQ_SIZE, reinterpret_cast<void (*)(void *)>(&SessionMgrThreadCallback), this);
    m_session_manager_thread->start();

    m_rtsp_msg_handler_thread = new MiracastThread(RTSP_HANDLER_THREAD_NAME, RTSP_HANDLER_THREAD_STACK, RTSP_HANDLER_MSG_COUNT, RTSP_HANDLER_MSGQ_SIZE, reinterpret_cast<void (*)(void *)>(&RTSPMsgHandlerCallback), this);
    m_rtsp_msg_handler_thread->start();

    m_p2p_ctrl_obj = MiracastP2P::getInstance();
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastController::destroy_ControllerFramework(void)
{
    MIRACASTLOG_TRACE("Entering...");
    MiracastP2P::destroyInstance();
    m_p2p_ctrl_obj = nullptr;

    delete m_rtsp_msg_handler_thread;
    delete m_session_manager_thread;
    delete m_plugin_req_handler_thread;
    delete m_rtsp_msg;

    m_rtsp_msg_handler_thread = nullptr;
    m_session_manager_thread = nullptr;
    m_plugin_req_handler_thread = nullptr;
    m_rtsp_msg = nullptr;
    MIRACASTLOG_TRACE("Exiting...");
}

std::string MiracastController::parse_p2p_event_data(const char *tmpBuff, const char *lookup_data)
{
    char return_buf[1024] = {0};
    const char *ret = nullptr, *ret_equal = nullptr, *ret_space = nullptr;
    ret = strstr(tmpBuff, lookup_data);
    if (nullptr != ret)
    {
        if (0 == strncmp(ret, lookup_data, strlen(lookup_data)))
        {
            ret_equal = strstr(ret, "=");
            ret_space = strstr(ret_equal, " ");
            if (ret_space)
            {
                snprintf(return_buf, (int)(ret_space - ret_equal), "%s", ret + strlen(lookup_data) + 1);
                MIRACASTLOG_VERBOSE("Parsed Data is - %s", return_buf);
            }
            else
            {
                snprintf(return_buf, strlen(ret_equal - 1), "%s", ret + strlen(lookup_data) + 1);
                MIRACASTLOG_VERBOSE("Parsed Data is - %s", return_buf);
            }
        }
    }
    if (return_buf != nullptr)
        return std::string(return_buf);
    else
        return std::string(" ");
}

std::string MiracastController::start_DHCPClient(std::string interface, std::string &default_gw_ip_addr)
{
    MIRACASTLOG_TRACE("Entering...");
    FILE *fp;
    std::string IP;
    char command[128] = {0};
    char data[1024] = {0};
    size_t len = 0;
    char *ln = nullptr;
    std::string local_addr = "";

    sprintf(command, "/sbin/udhcpc -v -i ");
    sprintf(command + strlen(command), interface.c_str());
    sprintf(command + strlen(command), " 2>&1");

    MIRACASTLOG_VERBOSE("command : [%s]", command);

    fp = popen(command, "r");

    if (!fp)
    {
        MIRACASTLOG_ERROR("Could not open pipe for output.");
    }
    else
    {
        while (getline(&ln, &len, fp) != -1)
        {
            sprintf(data + strlen(data), ln);
            MIRACASTLOG_VERBOSE("data : [%s]", data);
        }
        pclose(fp);

        std::string popen_buffer = data;

        MIRACASTLOG_VERBOSE("popen_buffer is %s\n", popen_buffer.c_str());

        std::string leaseof_str = "lease of ";
        std::string gw_str = "route add default gw ";

        std::size_t local_ip_pos = popen_buffer.find(leaseof_str.c_str()) + leaseof_str.length();
        local_addr = popen_buffer.substr(local_ip_pos, popen_buffer.find(" obtained") - local_ip_pos);
        MIRACASTLOG_VERBOSE("local IP addr obtained is %s\n", local_addr.c_str());

        /* Here retrieved the default gw ip address. Later it can be used as GO IP address if P2P-GROUP started as PERSISTENT */
        std::size_t gw_pos = popen_buffer.find(gw_str.c_str()) + gw_str.length();
        default_gw_ip_addr = popen_buffer.substr(gw_pos, popen_buffer.find(" dev") - gw_pos);
        MIRACASTLOG_VERBOSE("default_gw_ip_addr obtained is %s\n", default_gw_ip_addr.c_str());

        free(ln);
    }
    MIRACASTLOG_TRACE("Exiting...");
    return local_addr;
}

/*
 * Wait for data returned by the socket for specified time
 */
bool MiracastController::wait_data_timeout(int m_Sockfd, unsigned ms)
{
    struct timeval timeout = {0};
    fd_set readFDSet;

    FD_ZERO(&readFDSet);
    FD_SET(m_Sockfd, &readFDSet);

    timeout.tv_sec = (ms / 1000);
    timeout.tv_usec = ((ms % 1000) * 1000);

    if (select(m_Sockfd + 1, &readFDSet, nullptr, nullptr, &timeout) > 0)
    {
        return FD_ISSET(m_Sockfd, &readFDSet);
    }
    return false;
}

RTSP_STATUS MiracastController::receive_buffer_timedOut(int socket_fd, void *buffer, size_t buffer_len)
{
    int recv_return = -1;
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

#ifdef ENABLE_NON_BLOCKING
    if (!wait_data_timeout(socket_fd, SOCKET_WAIT_TIMEOUT_IN_MILLISEC))
    {
        return RTSP_TIMEDOUT;
    }
    else
    {
        recv_return = recv(socket_fd, buffer, buffer_len, 0);
    }
#else
    recv_return = read(socket_fd, buffer, buffer_len);
#endif

    if (recv_return <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            MIRACASTLOG_ERROR("error: recv timed out\n");
            status = RTSP_TIMEDOUT;
        }
        else
        {
            MIRACASTLOG_ERROR("error: recv failed, or connection closed\n");
            status = RTSP_MSG_FAILURE;
        }
    }
    MIRACASTLOG_INFO("received string(%d) - %s\n", recv_return, buffer);
    return status;
}

MiracastError MiracastController::initiate_TCP(std::string goIP)
{
    MIRACASTLOG_TRACE("Entering...");
    MiracastError ret = MIRACAST_FAIL;
    int r, i, num_ready = 0;
    size_t addr_size = 0;
    struct epoll_event events[MAX_EPOLL_EVENTS];
    struct sockaddr_in addr = {0};
    struct sockaddr_storage str_addr = {0};

    system("iptables -I INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");
    system("iptables -I OUTPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(7236);

    r = inet_pton(AF_INET, goIP.c_str(), &addr.sin_addr);

    if (r != 1)
    {
        MIRACASTLOG_ERROR("inet_issue");
        return MIRACAST_FAIL;
    }

    memcpy(&str_addr, &addr, sizeof(addr));
    addr_size = sizeof(addr);

    struct sockaddr_storage in_addr = str_addr;

    m_tcpSockfd = socket(in_addr.ss_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (m_tcpSockfd < 0)
    {
        MIRACASTLOG_ERROR("TCP Socket creation error %s", strerror(errno));
        return MIRACAST_FAIL;
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

    r = connect(m_tcpSockfd, (struct sockaddr *)&in_addr, addr_size);
    if (r < 0)
    {
        if (errno != EINPROGRESS)
        {
            MIRACASTLOG_INFO("Event %s received(%d)", strerror(errno), errno);
        }
        else
        {
#ifdef ENABLE_NON_BLOCKING
            // connection in progress
            if (!wait_data_timeout(m_tcpSockfd, SOCKET_WAIT_TIMEOUT_IN_MILLISEC))
            {
                // connection timed out or failed
                MIRACASTLOG_INFO("Socket Connection Timedout ...\n");
            }
            else
            {
                // connection successful
                // do something with the connected socket
                MIRACASTLOG_INFO("Socket Connected Successfully ...\n");
                ret = MIRACAST_OK;
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
        ret = MIRACAST_OK;
    }

    /*---Wait for socket connect to complete---*/
    num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000 /*timeout*/);
    for (i = 0; i < num_ready; i++)
    {
        if (events[i].events & EPOLLOUT)
        {
            MIRACASTLOG_INFO("Socket(%d) %d connected (EPOLLOUT)", i, events[i].data.fd);
        }
    }

    num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000 /*timeout*/);
    for (i = 0; i < num_ready; i++)
    {
        if (events[i].events & EPOLLOUT)
        {
            MIRACASTLOG_INFO("Socket %d got some data via EPOLLOUT", events[i].data.fd);
            return ret;
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return ret;
}

SESSION_MANAGER_ACTIONS MiracastController::convertP2PtoSessionActions(P2P_EVENTS eventId)
{
    SESSION_MANAGER_ACTIONS action_type = SESSION_MGR_INVALID_ACTION;

    switch (eventId)
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

void MiracastController::restart_session(void)
{
    MIRACASTLOG_TRACE("Entering...");
    stop_session();
    discover_devices();
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastController::stop_session(void)
{
    MIRACASTLOG_TRACE("Entering...");
    stop_discover_devices();
    if (m_groupInfo)
    {
        delete m_groupInfo;
        m_groupInfo = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastController::event_handler(P2P_EVENTS eventId, void *data, size_t len, bool isIARMEnabled)
{
    SESSION_MGR_MSG_STRUCT msg_buffer = {0};
    std::string event_buffer;
    MIRACASTLOG_VERBOSE("event received");
    if (isIARMEnabled)
    {
        IARM_BUS_WiFiSrvMgr_P2P_EventData_t *EventData = (IARM_BUS_WiFiSrvMgr_P2P_EventData_t *)data;
        event_buffer = EventData->event_data;
    }
    else
    {
        event_buffer = (char *)data;
        free(data);
    }
    msg_buffer.action = convertP2PtoSessionActions(eventId);
    strcpy(msg_buffer.event_buffer, event_buffer.c_str());
    MIRACASTLOG_INFO("event_handler to SessionMgr Action[%#04X] buffer:%s  ", msg_buffer.action, event_buffer.c_str());
    m_session_manager_thread->send_message(&msg_buffer, sizeof(msg_buffer));
    MIRACASTLOG_VERBOSE("event received : %d buffer:%s  ", eventId, event_buffer.c_str());
}

MiracastError MiracastController::set_WFDParameters(void)
{
    MIRACASTLOG_TRACE("Entering...");
    MiracastError ret = MIRACAST_FAIL;
    ret = m_p2p_ctrl_obj->set_WFDParameters();
    MIRACASTLOG_TRACE("Exiting...");
    return ret;
}

MiracastError MiracastController::discover_devices(void)
{
    MIRACASTLOG_TRACE("Entering...");
    MiracastError ret = MIRACAST_FAIL;
    ret = m_p2p_ctrl_obj->discover_devices();
    MIRACASTLOG_TRACE("Exiting...");
    return ret;
}

MiracastError MiracastController::stop_discover_devices(void)
{
    MIRACASTLOG_TRACE("Entering...");
    MiracastError ret = MIRACAST_FAIL;
    ret = m_p2p_ctrl_obj->stop_discover_devices();
    MIRACASTLOG_TRACE("Exiting...");
    return ret;
}

MiracastError MiracastController::connect_device(std::string MAC)
{
    MIRACASTLOG_TRACE("Entering...");
    MIRACASTLOG_VERBOSE("Connecting to the MAC - %s", MAC.c_str());
    MiracastError ret = MIRACAST_FAIL;
    ret = m_p2p_ctrl_obj->connect_device(MAC);
    MIRACASTLOG_TRACE("Exiting...");
    return ret;
}

MiracastError MiracastController::start_streaming()
{
    MIRACASTLOG_TRACE("Entering...");
    const char *mcastIptableFile = "/opt/mcast_iptable.txt";
    std::ifstream mIpfile(mcastIptableFile);
    std::string mcast_iptable;
    if (mIpfile.is_open())
    {
        std::getline(mIpfile, mcast_iptable);
        MIRACASTLOG_INFO("Iptable reading from file [%s] as [ %s] ", mcastIptableFile, mcast_iptable.c_str());
        system(mcast_iptable.c_str());
        mIpfile.close();
    }
    else
    {
        system("iptables -I INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
    }
    MIRACASTLOG_INFO("Casting started. Player initiated");
    std::string gstreamerPipeline;

    const char *mcastfile = "/opt/mcastgstpipline.txt";
    std::ifstream mcgstfile(mcastfile);

    if (mcgstfile.is_open())
    {
        std::getline(mcgstfile, gstreamerPipeline);
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
        if (access("/opt/miracast_gst", F_OK) == 0)
        {
            gstreamerPipeline = "GST_DEBUG=3 gst-launch-1.0 -vvv playbin uri=udp://0.0.0.0:1990 video-sink=\"westerossink\"";
            MIRACASTLOG_INFO("pipeline constructed is --> %s", gstreamerPipeline.c_str());
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
            MiracastPlayer *miracastPlayerObj = MiracastPlayer::getInstance();
            std::string port = get_wfd_streaming_port_number();
            std::string local_ip = get_localIp();
            miracastPlayerObj->launch(local_ip, port);
        }
    }

    std::string MAC = m_rtsp_msg->get_WFDSourceMACAddress();
    std::string device_name = m_rtsp_msg->get_WFDSourceName();

    m_notify_handler->onMiracastServiceClientConnectionStarted(MAC, device_name);
    MIRACASTLOG_TRACE("Exiting...");
    return MIRACAST_OK;
}

MiracastError MiracastController::stop_streaming()
{
    system("iptables -D INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT");
    MiracastPlayer *miracastPlayerObj = MiracastPlayer::getInstance();
    miracastPlayerObj->stop();
    MIRACASTLOG_INFO("Stopped Streaming.");
    return MIRACAST_OK;
}

MiracastError MiracastController::disconnect_device()
{
    MiracastPlayer *miracastPlayerObj = MiracastPlayer::getInstance();
    miracastPlayerObj->stop();
    return MIRACAST_OK;
}

std::string MiracastController::get_localIp()
{
    std::string ip_addr = "";
    if (nullptr != m_groupInfo)
    {
        ip_addr = m_groupInfo->localIPAddr;
    }
    return ip_addr;
}

std::string MiracastController::get_wfd_streaming_port_number()
{
    return m_rtsp_msg->get_WFDStreamingPortNumber();
}

std::string MiracastController::get_connected_device_mac()
{
    return m_rtsp_msg->get_WFDSourceMACAddress();
}

std::vector<DeviceInfo *> MiracastController::get_allPeers()
{
    return m_deviceInfoList;
}

bool MiracastController::get_connection_status()
{
    return m_connectionStatus;
}

DeviceInfo *MiracastController::get_device_details(std::string MAC)
{
    DeviceInfo *deviceInfo = nullptr;
    std::size_t found;
    // memset(deviceInfo, 0, sizeof(deviceInfo));
    for (auto device : m_deviceInfoList)
    {
        found = device->deviceMAC.find(MAC);
        if (found != std::string::npos)
        {
            deviceInfo = device;
            break;
        }
    }
    return deviceInfo;
}

RTSP_STATUS MiracastController::send_rstp_msg(int socket_fd, std::string rtsp_response_buffer)
{
    int read_ret = 0;
    read_ret = send(socket_fd, rtsp_response_buffer.c_str(), rtsp_response_buffer.length(), 0);

    if (0 > read_ret)
    {
        MIRACASTLOG_INFO("Send Failed (%d)%s", errno, strerror(errno));
        return RTSP_MSG_FAILURE;
    }

    MIRACASTLOG_INFO("Sending the RTSP response %d Data[%s]\n", read_ret, rtsp_response_buffer.c_str());
    return RTSP_MSG_SUCCESS;
}

RTSP_STATUS MiracastController::validate_rtsp_m1_msg_m2_send_request(std::string rtsp_m1_msg_buffer)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    size_t found = rtsp_m1_msg_buffer.find(RTSP_REQ_OPTIONS);
    MIRACASTLOG_TRACE("Entering...");
    MIRACASTLOG_INFO("M1 request received");
    if (found != std::string::npos)
    {
        std::string m1_msg_resp_sink2src = "";
        MIRACASTLOG_INFO("M1 OPTIONS packet received");
        std::stringstream ss(rtsp_m1_msg_buffer);
        std::string prefix = "";
        std::string req_str;
        std::string seq_str;
        std::string line;

        while (std::getline(ss, line))
        {
            if (line.find(RTSP_STD_REQUIRE_FIELD) != std::string::npos)
            {
                prefix = RTSP_STD_REQUIRE_FIELD;
                req_str = line.substr(prefix.length());
                REMOVE_R(req_str);
                REMOVE_N(req_str);
            }
            else if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos)
            {
                prefix = RTSP_STD_SEQUENCE_FIELD;
                seq_str = line.substr(prefix.length());
                REMOVE_R(seq_str);
                REMOVE_N(seq_str);
            }
        }

        m1_msg_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M1_RESPONSE, seq_str, req_str);

        MIRACASTLOG_INFO("Sending the M1 response \n-%s", m1_msg_resp_sink2src.c_str());

        status_code = send_rstp_msg(m_tcpSockfd, m1_msg_resp_sink2src);

        if (RTSP_MSG_SUCCESS == status_code)
        {
            std::string m2_msg_req_sink2src = "";
            MIRACASTLOG_INFO("M1 response sent\n");

            m2_msg_req_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M2_REQUEST, empty_string, req_str);

            MIRACASTLOG_INFO("%s", m2_msg_req_sink2src.c_str());
            MIRACASTLOG_INFO("Sending the M2 request \n");
            status_code = send_rstp_msg(m_tcpSockfd, m2_msg_req_sink2src);
            if (RTSP_MSG_SUCCESS == status_code)
            {
                MIRACASTLOG_INFO("M2 request sent\n");
            }
            else
            {
                MIRACASTLOG_ERROR("M2 request failed\n");
            }
        }
        else
        {
            MIRACASTLOG_ERROR("M1 Response failed\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return (status_code);
}

RTSP_STATUS MiracastController::validate_rtsp_m2_request_ack(std::string rtsp_m1_response_ack_buffer)
{
    // Yet to check and validate the response
    return RTSP_MSG_SUCCESS;
}

RTSP_STATUS MiracastController::validate_rtsp_m3_response_back(std::string rtsp_m3_msg_buffer)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    MIRACASTLOG_TRACE("Entering...");
    MIRACASTLOG_INFO("M3 request received");

    if (rtsp_m3_msg_buffer.find("wfd_video_formats") != std::string::npos)
    {
        std::string m3_msg_resp_sink2src = "";
        std::string seq_str = "";
        std::stringstream ss(rtsp_m3_msg_buffer);
        std::string prefix = "";
        std::string line;

        while (std::getline(ss, line))
        {
            if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos)
            {
                prefix = RTSP_STD_SEQUENCE_FIELD;
                seq_str = line.substr(prefix.length());
                REMOVE_R(seq_str);
                REMOVE_N(seq_str);
                break;
            }
        }

        std::string content_buffer;

        m3_msg_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M3_RESPONSE, seq_str, empty_string);

        MIRACASTLOG_INFO("%s", m3_msg_resp_sink2src.c_str());

        status_code = send_rstp_msg(m_tcpSockfd, m3_msg_resp_sink2src);

        if (RTSP_MSG_SUCCESS == status_code)
        {
            MIRACASTLOG_INFO("Sending the M3 response \n");
        }
        else
        {
            MIRACASTLOG_ERROR("Sending the M3 response Failed\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return (status_code);
}

RTSP_STATUS MiracastController::validate_rtsp_m4_response_back(std::string rtsp_m4_msg_buffer)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    MIRACASTLOG_TRACE("Entering...");
    if (rtsp_m4_msg_buffer.find("SET_PARAMETER") != std::string::npos)
    {
        std::string m4_msg_resp_sink2src = "";
        std::string seq_str = "";
        std::string url = "";
        std::stringstream ss(rtsp_m4_msg_buffer);
        std::string prefix = "";
        std::string line;

        while (std::getline(ss, line))
        {
            if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos)
            {
                prefix = RTSP_STD_SEQUENCE_FIELD;
                seq_str = line.substr(prefix.length());
                REMOVE_R(seq_str);
                REMOVE_N(seq_str);
            }
            else if (line.find(RTSP_WFD_PRESENTATION_URL_FIELD) != std::string::npos)
            {
                prefix = RTSP_WFD_PRESENTATION_URL_FIELD;
                std::size_t url_start_pos = line.find(prefix) + prefix.length();
                std::size_t url_end_pos = line.find(RTSP_SPACE_STR, url_start_pos);
                url = line.substr(url_start_pos, url_end_pos - url_start_pos);
                m_rtsp_msg->set_WFDPresentationURL(url);
            }
        }

        m4_msg_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M4_RESPONSE, seq_str, empty_string);

        MIRACASTLOG_INFO("Sending the M4 response \n");
        status_code = send_rstp_msg(m_tcpSockfd, m4_msg_resp_sink2src);
        if (RTSP_MSG_SUCCESS == status_code)
        {
            MIRACASTLOG_INFO("M4 response sent\n");
        }
        else
        {
            MIRACASTLOG_INFO("Failed to sent M4 response\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return (status_code);
}

RTSP_STATUS MiracastController::validate_rtsp_m5_msg_m6_send_request(std::string rtsp_m5_msg_buffer)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    MIRACASTLOG_TRACE("Entering...");
    if (rtsp_m5_msg_buffer.find("wfd_trigger_method: SETUP") != std::string::npos)
    {
        std::string m5_msg_resp_sink2src = "";
        std::string seq_str = "";
        std::stringstream ss(rtsp_m5_msg_buffer);
        std::string prefix = "";
        std::string line;

        while (std::getline(ss, line))
        {
            if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos)
            {
                prefix = RTSP_STD_SEQUENCE_FIELD;
                seq_str = line.substr(prefix.length());
                REMOVE_R(seq_str);
                REMOVE_N(seq_str);
                break;
            }
        }

        m5_msg_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M5_RESPONSE, seq_str, empty_string);

        MIRACASTLOG_INFO("Sending the M5 response \n");
        status_code = send_rstp_msg(m_tcpSockfd, m5_msg_resp_sink2src);
        if (RTSP_MSG_SUCCESS == status_code)
        {
            std::string m6_msg_req_sink2src = "";
            MIRACASTLOG_INFO("M5 Response has sent\n");

            m6_msg_req_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M6_REQUEST, seq_str, empty_string);

            MIRACASTLOG_INFO("Sending the M6 Request\n");
            status_code = send_rstp_msg(m_tcpSockfd, m6_msg_req_sink2src);
            if (RTSP_MSG_SUCCESS == status_code)
            {
                MIRACASTLOG_INFO("M6 Request has sent\n");
            }
            else
            {
                MIRACASTLOG_ERROR("Failed to Send the M6 Request\n");
            }
        }
        else
        {
            MIRACASTLOG_ERROR("Failed to Send the M5 response\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return (status_code);
}

RTSP_STATUS MiracastController::validate_rtsp_m6_ack_m7_send_request(std::string rtsp_m6_ack_buffer)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    MIRACASTLOG_TRACE("Entering...");
    if (!rtsp_m6_ack_buffer.empty())
    {
        size_t pos_ses = rtsp_m6_ack_buffer.find(RTSP_STD_SESSION_FIELD);
        std::string session = rtsp_m6_ack_buffer.substr(pos_ses + strlen(RTSP_STD_SESSION_FIELD));
        pos_ses = session.find(";");
        std::string session_number = session.substr(0, pos_ses);

        m_rtsp_msg->set_WFDSessionNumber(session_number);

        if (rtsp_m6_ack_buffer.find("client_port") != std::string::npos)
        {
            std::string m7_msg_req_sink2src = "";

            m7_msg_req_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M7_REQUEST, empty_string, empty_string);

            MIRACASTLOG_INFO("Sending the M7 Request\n");
            status_code = send_rstp_msg(m_tcpSockfd, m7_msg_req_sink2src);
            if (RTSP_MSG_SUCCESS == status_code)
            {
                MIRACASTLOG_INFO("M7 Request has sent\n");
            }
            else
            {
                MIRACASTLOG_ERROR("Failed to Send the M7 Request\n");
            }
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return (status_code);
}

RTSP_STATUS MiracastController::validate_rtsp_m7_request_ack(std::string rtsp_m7_ack_buffer)
{
    // Yet to check and validate the M7 acknowledgement
    return RTSP_MSG_SUCCESS;
}

RTSP_STATUS MiracastController::validate_rtsp_post_m1_m7_xchange(std::string rtsp_post_m1_m7_xchange_buffer)
{
    MIRACASTLOG_TRACE("Entering...");
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    RTSP_STATUS sub_status_code = RTSP_MSG_SUCCESS;
    std::string rtsp_resp_sink2src = "";
    std::string seq_str = "";
    std::stringstream ss(rtsp_post_m1_m7_xchange_buffer);
    std::string prefix = "";
    std::string line;

    while (std::getline(ss, line))
    {
        if (line.find(RTSP_STD_SEQUENCE_FIELD) != std::string::npos)
        {
            prefix = RTSP_STD_SEQUENCE_FIELD;
            seq_str = line.substr(prefix.length());
            REMOVE_R(seq_str);
            REMOVE_N(seq_str);
            break;
        }
    }

    if (rtsp_post_m1_m7_xchange_buffer.find(RTSP_M16_REQUEST_MSG) != std::string::npos)
    {
        rtsp_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_M16_RESPONSE, seq_str, empty_string);
    }
    else if (rtsp_post_m1_m7_xchange_buffer.find(RTSP_REQ_TEARDOWN_MODE) != std::string::npos)
    {
        MIRACASTLOG_INFO("TEARDOWN request from Source received\n");
        rtsp_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_TEARDOWN_RESPONSE, seq_str, empty_string);
        sub_status_code = RTSP_MSG_TEARDOWN_REQUEST;
    }

    if (!(rtsp_resp_sink2src.empty()))
    {
        MIRACASTLOG_INFO("Sending the Response \n");
        status_code = send_rstp_msg(m_tcpSockfd, rtsp_resp_sink2src);
        if (RTSP_MSG_SUCCESS == status_code)
        {
            status_code = sub_status_code;
            MIRACASTLOG_INFO("Response sent\n");
        }
        else
        {
            MIRACASTLOG_INFO("Failed to sent Response\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return status_code;
}

RTSP_STATUS MiracastController::rtsp_sink2src_request_msg_handling(RTSP_MSG_HANDLER_ACTIONS action_id)
{
    MIRACASTLOG_TRACE("Entering...");
    RTSP_MSG_FMT_SINK2SRC request_mode = RTSP_MSG_FMT_INVALID;
    RTSP_STATUS status_code = RTSP_MSG_SUCCESS;

    switch (action_id)
    {
    case RTSP_TEARDOWN_FROM_SINK2SRC:
    {
        request_mode = RTSP_MSG_FMT_TEARDOWN_REQUEST;
    }
    break;
    case RTSP_PLAY_FROM_SINK2SRC:
    {
        request_mode = RTSP_MSG_FMT_PLAY_REQUEST;
    }
    break;
    case RTSP_PAUSE_FROM_SINK2SRC:
    {
        request_mode = RTSP_MSG_FMT_PAUSE_REQUEST;
    }
    break;
    default:
    {
        //
    }
    break;
    }

    if (RTSP_MSG_FMT_INVALID != request_mode)
    {
        std::string rtsp_resp_sink2src;
        rtsp_resp_sink2src = m_rtsp_msg->generate_request_response_msg(RTSP_MSG_FMT_TEARDOWN_RESPONSE, empty_string, empty_string);

        if (!(rtsp_resp_sink2src.empty()))
        {
            MIRACASTLOG_INFO("Sending the [PLAY/PAUSE/TEARDOWN] REQUEST \n");
            status_code = send_rstp_msg(m_tcpSockfd, rtsp_resp_sink2src);
            if (RTSP_MSG_SUCCESS == status_code)
            {
                MIRACASTLOG_INFO("[PLAY/PAUSE/TEARDOWN] sent\n");
            }
            else
            {
                MIRACASTLOG_INFO("Failed to sent PLAY/PAUSE\n");
            }
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return status_code;
}

RTSP_STATUS MiracastController::validate_rtsp_msg_response_back(std::string rtsp_msg_buffer, RTSP_MSG_HANDLER_ACTIONS action_id)
{
    RTSP_STATUS status_code = RTSP_INVALID_MSG_RECEIVED;
    MIRACASTLOG_TRACE("Entering...");
    switch (action_id)
    {
    case RTSP_M1_REQUEST_RECEIVED:
    {
        status_code = validate_rtsp_m1_msg_m2_send_request(rtsp_msg_buffer);
    }
    break;
    case RTSP_M2_REQUEST_ACK:
    {
        status_code = validate_rtsp_m2_request_ack(rtsp_msg_buffer);
    }
    break;
    case RTSP_M3_REQUEST_RECEIVED:
    {
        status_code = validate_rtsp_m3_response_back(rtsp_msg_buffer);
    }
    break;
    case RTSP_M4_REQUEST_RECEIVED:
    {
        status_code = validate_rtsp_m4_response_back(rtsp_msg_buffer);
    }
    break;
    case RTSP_M5_REQUEST_RECEIVED:
    {
        status_code = validate_rtsp_m5_msg_m6_send_request(rtsp_msg_buffer);
    }
    break;
    case RTSP_M6_REQUEST_ACK:
    {
        status_code = validate_rtsp_m6_ack_m7_send_request(rtsp_msg_buffer);
    }
    break;
    case RTSP_M7_REQUEST_ACK:
    {
        status_code = validate_rtsp_m7_request_ack(rtsp_msg_buffer);
    }
    break;
    case RTSP_MSG_POST_M1_M7_XCHANGE:
    {
        status_code = validate_rtsp_post_m1_m7_xchange(rtsp_msg_buffer);
    }
    break;
    default:
    {
        //
    }
    }
    MIRACASTLOG_INFO("Validating RTSP Msg => ACTION[%#04X] Resp[%#04X]\n", action_id, status_code);
    MIRACASTLOG_TRACE("Exiting...");
    return status_code;
}

MiracastThread::MiracastThread(std::string thread_name, size_t stack_size, size_t msg_size, size_t queue_depth, void (*callback)(void *), void *user_data)
{
    MIRACASTLOG_TRACE("Entering...");
    thread_name = thread_name;

    thread_stacksize = stack_size;
    thread_message_size = msg_size;
    thread_message_count = queue_depth;

    thread_user_data = user_data;

    thread_callback = callback;

    // Create message queue
    g_queue = g_async_queue_new();
    // g_async_queue_ref( g_queue );

    sem_init(&sem_object, 0, 0);

    // Create thread
    pthread_attr_init(&attr_);
    pthread_attr_setstacksize(&attr_, stack_size);
    MIRACASTLOG_TRACE("Exiting...");
}

MiracastThread::~MiracastThread()
{
    MIRACASTLOG_TRACE("Entering...");
    // Join thread
    pthread_join(thread_, nullptr);

    // Close message queue
    g_async_queue_unref(g_queue);
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastThread::start(void)
{
    MIRACASTLOG_TRACE("Entering...");
    pthread_create(&thread_, &attr_, reinterpret_cast<void *(*)(void *)>(thread_callback), thread_user_data);
    MIRACASTLOG_TRACE("Exiting...");
}

void MiracastThread::send_message(void *message, size_t msg_size)
{
    MIRACASTLOG_TRACE("Entering...");
    void *buffer = malloc(msg_size);
    if (nullptr == buffer)
    {
        MIRACASTLOG_ERROR("Memory Allocation Failed for %u\n", msg_size);
        return;
    }
    memset(buffer, 0x00, msg_size);
    // Send message to queue

    memcpy(buffer, message, msg_size);
    g_async_queue_push(this->g_queue, buffer);
    sem_post(&this->sem_object);
    MIRACASTLOG_TRACE("Exiting...");
}

int8_t MiracastThread::receive_message(void *message, size_t msg_size, int sem_wait_timedout)
{
    int8_t status = false;
    MIRACASTLOG_TRACE("Entering...");
    if (MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT == sem_wait_timedout)
    {
        sem_wait(&this->sem_object);
        status = true;
    }
    else if (0 < sem_wait_timedout)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += sem_wait_timedout;

        if (-1 != sem_timedwait(&sem_object, &ts))
        {
            status = true;
        }
    }
    else
    {
        status = -1;
    }

    if ((true == status) && (nullptr != this->g_queue))
    {
        void *data_ptr = static_cast<void *>(g_async_queue_pop(this->g_queue));
        if ((nullptr != message) && (nullptr != data_ptr))
        {
            memcpy(message, data_ptr, msg_size);
            free(data_ptr);
        }
    }
    MIRACASTLOG_TRACE("Exiting...");
    return status;
}

std::string MiracastController::get_device_name(std::string mac_address)
{
    MIRACASTLOG_TRACE("Entering...");
    size_t found;
    std::string device_name = "";
    int i = 0;
    for (auto devices : m_deviceInfoList)
    {
        found = devices->deviceMAC.find(mac_address);
        if (found != std::string::npos)
        {
            device_name = devices->modelName;
            break;
        }
        i++;
    }
    MIRACASTLOG_TRACE("Exiting...");
    return device_name;
}

MiracastError MiracastController::set_FriendlyName(std::string friendly_name)
{
    MiracastError ret = MIRACAST_OK;
    MIRACASTLOG_TRACE("Entering..");

    ret = m_p2p_ctrl_obj->set_FriendlyName(friendly_name, true);

    MIRACASTLOG_TRACE("Exiting..");
    return ret;
}

std::string MiracastController::get_FriendlyName(void)
{
    MIRACASTLOG_TRACE("Entering and Exiting...");

    return m_p2p_ctrl_obj->get_FriendlyName();
}

void MiracastController::SessionManager_Thread(void *args)
{
    SESSION_MGR_MSG_STRUCT session_message_data = {0};
    RTSP_HLDR_MSG_STRUCT rtsp_message_data = {RTSP_INVALID_ACTION, 0};
    bool plugin_req_client_connection_sent = false;

    while (true)
    {
        std::string event_buffer;
        event_buffer.clear();

        MIRACASTLOG_TRACE("[%s] Waiting for Event .....\n", __FUNCTION__);
        m_session_manager_thread->receive_message(&session_message_data, SESSION_MGR_MSGQ_SIZE, MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT);

        event_buffer = session_message_data.event_buffer;

        MIRACASTLOG_TRACE("[%s] Received Action[%#04X]Data[%s]\n", __FUNCTION__, session_message_data.action, event_buffer.c_str());

        if (SESSION_MGR_SELF_ABORT == session_message_data.action)
        {
            MIRACASTLOG_TRACE("SESSION_MGR_SELF_ABORT Received.\n");
            rtsp_message_data.action = RTSP_SELF_ABORT;
            m_rtsp_msg_handler_thread->send_message(&rtsp_message_data, RTSP_HANDLER_MSGQ_SIZE);
            stop_session();
            break;
        }

        switch (session_message_data.action)
        {
        case SESSION_MGR_START_DISCOVERING:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_START_DISCOVERING Received\n");
            set_WFDParameters();
            discover_devices();
        }
        break;
        case SESSION_MGR_STOP_DISCOVERING:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_STOP_DISCOVERING Received\n");
            stop_session();
        }
        break;
        case SESSION_MGR_GO_DEVICE_FOUND:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_GO_DEVICE_FOUND Received\n");
            std::string wfdSubElements;
            DeviceInfo *device = new DeviceInfo;
            device->deviceMAC = parse_p2p_event_data(event_buffer.c_str(), "p2p_dev_addr");
            device->deviceType = parse_p2p_event_data(event_buffer.c_str(), "pri_dev_type");
            device->modelName = parse_p2p_event_data(event_buffer.c_str(), "name");
            wfdSubElements = parse_p2p_event_data(event_buffer.c_str(), "wfd_dev_info");
#if 0
				device->isCPSupported = ((strtol(wfdSubElements.c_str(), nullptr, 16) >> 32) && 256);
				device->deviceRole = (DEVICEROLE)((strtol(wfdSubElements.c_str(), nullptr, 16) >> 32) && 3);
#endif
            MIRACASTLOG_TRACE("Device data parsed & stored successfully");

            m_deviceInfoList.push_back(device);
        }
        break;
        case SESSION_MGR_GO_DEVICE_LOST:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_GO_DEVICE_LOST Received\n");
            std::string lostMAC = parse_p2p_event_data(event_buffer.c_str(), "p2p_dev_addr");
            size_t found;
            int i = 0;
            for (auto devices : m_deviceInfoList)
            {
                found = devices->deviceMAC.find(lostMAC);
                if (found != std::string::npos)
                {
                    delete devices;
                    m_deviceInfoList.erase(m_deviceInfoList.begin() + i);
                    break;
                }
                i++;
            }
        }
        break;
        case SESSION_MGR_GO_DEVICE_PROVISION:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_GO_DEVICE_PROVISION Received\n");
            // m_authType = "pbc";
            std::string MAC = parse_p2p_event_data(event_buffer.c_str(), "p2p_dev_addr");
        }
        break;
        case SESSION_MGR_GO_NEG_REQUEST:
        {
            PLUGIN_REQ_HDLR_MSG_STRUCT plugin_req_msg_data = {0};
            MIRACASTLOG_TRACE("SESSION_MGR_GO_NEG_REQUEST Received\n");
            std::string MAC;
            size_t space_find = event_buffer.find(" ");
            size_t dev_str = event_buffer.find("dev_passwd_id");
            if ((space_find != std::string::npos) && (dev_str != std::string::npos))
            {
                MAC = event_buffer.substr(space_find, dev_str - space_find);
                REMOVE_SPACES(MAC);
            }

            std::string device_name = get_device_name(MAC);

            if (false == plugin_req_client_connection_sent)
            {
                if (m_rtsp_msg->get_WFDSourceMACAddress().empty())
                {
                    plugin_req_msg_data.action = PLUGIN_REQ_HLDR_CONNECT_DEVICE_FROM_SESSION_MGR;
                    strcpy(plugin_req_msg_data.action_buffer, MAC.c_str());
                    strcpy(plugin_req_msg_data.buffer_user_data, device_name.c_str());
                    m_plugin_req_handler_thread->send_message(&plugin_req_msg_data, sizeof(plugin_req_msg_data));
                    plugin_req_client_connection_sent = true;
                }
                else
                {
                    // TODO
                    //  Need to handle connect request received evenafter connection already established with other client
                }
            }
            else
            {
                // TODO
                //  Need to handle connect request received evenafter connection already established with other client
            }
        }
        break;
        case SESSION_MGR_CONNECT_REQ_FROM_HANDLER:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_CONNECT_REQ_FROM_HANDLER Received\n");
            std::string mac_address = event_buffer;
            std::string device_name = get_device_name(mac_address);

            connect_device(mac_address);
            m_rtsp_msg->set_WFDSourceMACAddress(mac_address);
            m_rtsp_msg->set_WFDSourceName(device_name);
            plugin_req_client_connection_sent = false;
        }
        break;
        case SESSION_MGR_GO_GROUP_STARTED:
        {
            std::string device_name = "";
            MIRACASTLOG_TRACE("SESSION_MGR_GO_GROUP_STARTED Received\n");
            m_groupInfo = new GroupInfo;
            MiracastError ret = MIRACAST_FAIL;
            size_t found = event_buffer.find("client");
            size_t found_space = event_buffer.find(" ");
            if (found != std::string::npos)
            {
                m_groupInfo->ipAddr = parse_p2p_event_data(event_buffer.c_str(), "ip_addr");
                m_groupInfo->ipMask = parse_p2p_event_data(event_buffer.c_str(), "ip_mask");
                m_groupInfo->goIPAddr = parse_p2p_event_data(event_buffer.c_str(), "go_ip_addr");
                m_groupInfo->goDevAddr = parse_p2p_event_data(event_buffer.c_str(), "go_dev_addr");
                m_groupInfo->SSID = parse_p2p_event_data(event_buffer.c_str(), "ssid");

                device_name = get_device_name(m_groupInfo->goDevAddr);

                size_t found_client = event_buffer.find("client");
                m_groupInfo->interface = event_buffer.substr(found_space, found_client - found_space);
                REMOVE_SPACES(m_groupInfo->interface);

                if (getenv("GET_PACKET_DUMP") != nullptr)
                {
                    std::string tcpdump;
                    tcpdump.append("tcpdump -i ");
                    tcpdump.append(m_groupInfo->interface);
                    tcpdump.append(" -s 65535 -w /opt/dump.pcap &");
                    MIRACASTLOG_VERBOSE("Dump command to execute - %s", tcpdump.c_str());
                    system(tcpdump.c_str());
                }

                std::string default_gw_ip = "";

                // STB is a client in the p2p group
                m_groupInfo->isGO = false;
                m_groupInfo->localIPAddr = start_DHCPClient(m_groupInfo->interface, default_gw_ip);
                if (m_groupInfo->localIPAddr.empty())
                {
                    MIRACASTLOG_ERROR("Local IP address is not obtained");
                    m_notify_handler->onMiracastServiceClientConnectionError(m_groupInfo->goDevAddr, device_name);
                    continue;
                }
                else
                {
                    if (m_groupInfo->goIPAddr.empty())
                    {
                        MIRACASTLOG_VERBOSE("default_gw_ip [%s]\n", default_gw_ip.c_str());
                        m_groupInfo->goIPAddr.append(default_gw_ip);
                    }
                    MIRACASTLOG_VERBOSE("initiate_TCP started GO IP[%s]\n", m_groupInfo->goIPAddr.c_str());
                    ret = initiate_TCP(m_groupInfo->goIPAddr);
                    MIRACASTLOG_VERBOSE("initiate_TCP done ret[%x]\n", ret);
                }

                if (MIRACAST_OK == ret)
                {
                    rtsp_message_data.action = RTSP_START_RECEIVE_MSGS;
                    MIRACASTLOG_TRACE("RTSP Thread Initialated with RTSP_START_RECEIVE_MSGS\n");
                    m_rtsp_msg_handler_thread->send_message(&rtsp_message_data, RTSP_HANDLER_MSGQ_SIZE);
                    ret = MIRACAST_FAIL;
                }
                else
                {
                    MIRACASTLOG_FATAL("TCP connection Failed");
                    m_notify_handler->onMiracastServiceClientConnectionError(m_groupInfo->goDevAddr, device_name);
                    continue;
                }
            }
            else
            {
                size_t found_go = event_buffer.find("GO");
                m_groupInfo->interface = event_buffer.substr(found_space, found_go - found_space);
                // STB is the GO in the p2p group
                m_groupInfo->isGO = true;
            }
            m_connectionStatus = true;
        }
        break;
        case SESSION_MGR_GO_GROUP_REMOVED:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_GO_GROUP_REMOVED Received\n");
            stop_streaming();
        }
        break;
        case SESSION_MGR_START_STREAMING:
        case SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY:
        {
            MIRACASTLOG_TRACE("[SESSION_MGR_START_STREAMING/SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY] Received\n");
            start_streaming();
        }
        break;
        case SESSION_MGR_PAUSE_STREAMING:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_PAUSE_STREAMING Received\n");
        }
        break;
        case SESSION_MGR_STOP_STREAMING:
        {
            MIRACASTLOG_TRACE("SESSION_MGR_STOP_STREAMING Received\n");
            stop_streaming();
        }
        break;
        case SESSION_MGR_RTSP_MSG_TIMEDOUT:
        case SESSION_MGR_RTSP_INVALID_MESSAGE:
        case SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED:
        case SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED:
        case SESSION_MGR_GO_NEG_FAILURE:
        case SESSION_MGR_GO_GROUP_FORMATION_FAILURE:
        case SESSION_MGR_RESTART_DISCOVERING:
        {
            std::string MAC = m_rtsp_msg->get_WFDSourceMACAddress();
            std::string device_name = m_rtsp_msg->get_WFDSourceName();

            if (SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED == session_message_data.action)
            {
                MIRACASTLOG_TRACE("[TEARDOWN_REQ] Received\n");
                m_notify_handler->onMiracastServiceClientStopRequest(MAC, device_name);
            }
            else if (SESSION_MGR_RESTART_DISCOVERING != session_message_data.action)
            {
                MIRACASTLOG_TRACE("[TIMEDOUT/SEND_REQ_RESP_FAIL/INVALID_MESSAG/GO_NEG/GROUP_FORMATION_FAILURE] Received\n");
                m_notify_handler->onMiracastServiceClientConnectionError(MAC, device_name);
            }
            m_rtsp_msg->reset_WFDSourceMACAddress();
            m_rtsp_msg->reset_WFDSourceName();
            restart_session();
        }
        break;
        case SESSION_MGR_CONNECT_REQ_REJECT:
        case SESSION_MGR_CONNECT_REQ_TIMEOUT:
        {
            plugin_req_client_connection_sent = false;
        }
        break;
        case SESSION_MGR_TEARDOWN_REQ_FROM_HANDLER:
        {
            rtsp_message_data.action = RTSP_TEARDOWN_FROM_SINK2SRC;
            MIRACASTLOG_TRACE("TEARDOWN request sent to RTSP handler\n");
            m_rtsp_msg_handler_thread->send_message(&rtsp_message_data, RTSP_HANDLER_MSGQ_SIZE);
            stop_streaming();
        }
        break;
        case SESSION_MGR_GO_STOP_FIND:
        case SESSION_MGR_GO_NEG_SUCCESS:
        case SESSION_MGR_GO_GROUP_FORMATION_SUCCESS:
        case SESSION_MGR_SELF_ABORT:
        {
            MIRACASTLOG_TRACE("[STOP_FIND/NEG_SUCCESS/GROUP_FORMATION_SUCCESS] Received\n");
        }
        break;
        case SESSION_MGR_GO_EVENT_ERROR:
        case SESSION_MGR_GO_UNKNOWN_EVENT:
        case SESSION_MGR_INVALID_ACTION:
        default:
        {
            MIRACASTLOG_ERROR("[GO_EVENT_ERROR/GO_UNKNOWN_EVENT/INVALID_ACTION] Received\n");
        }
        break;
        }
    }
}

void MiracastController::RTSPMessageHandler_Thread(void *args)
{
    char rtsp_message_socket[4096] = {0};
    RTSP_HLDR_MSG_STRUCT rtsp_message_data = {};
    SESSION_MGR_MSG_STRUCT session_mgr_buffer = {0};
    RTSP_STATUS status_code = RTSP_TIMEDOUT;
    std::string socket_buffer;
    bool start_monitor_keep_alive_msg = false;

    while (true)
    {
        MIRACASTLOG_TRACE("[%s] Waiting for Event .....\n", __FUNCTION__);
        m_rtsp_msg_handler_thread->receive_message(&rtsp_message_data, sizeof(rtsp_message_data), MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT);

        MIRACASTLOG_TRACE("[%s] Received Action[%#04X]\n", __FUNCTION__, rtsp_message_data.action);

        if (RTSP_SELF_ABORT == rtsp_message_data.action)
        {
            MIRACASTLOG_TRACE("RTSP_SELF_ABORT ACTION Received\n");
            break;
        }

        if (RTSP_START_RECEIVE_MSGS != rtsp_message_data.action)
        {
            continue;
        }
        rtsp_message_data.action = RTSP_M1_REQUEST_RECEIVED;

        memset(&rtsp_message_socket, 0x00, sizeof(rtsp_message_socket));

        while (RTSP_MSG_SUCCESS == receive_buffer_timedOut(m_tcpSockfd, rtsp_message_socket, sizeof(rtsp_message_socket)))
        {
            socket_buffer.clear();
            socket_buffer = rtsp_message_socket;

            status_code = validate_rtsp_msg_response_back(socket_buffer, rtsp_message_data.action);

            MIRACASTLOG_TRACE("[%s] Validate RTSP Msg Action[%#04X] Response[%#04X]\n", __FUNCTION__, rtsp_message_data.action, status_code);

            if ((RTSP_MSG_SUCCESS != status_code) || (RTSP_M7_REQUEST_ACK == rtsp_message_data.action))
            {
                break;
            }
            memset(&rtsp_message_socket, 0x00, sizeof(rtsp_message_socket));
            rtsp_message_data.action = static_cast<RTSP_MSG_HANDLER_ACTIONS>(rtsp_message_data.action + 1);
        }

        start_monitor_keep_alive_msg = false;

        if ((RTSP_MSG_SUCCESS == status_code) && (RTSP_M7_REQUEST_ACK == rtsp_message_data.action))
        {
            session_mgr_buffer.action = SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY;
            start_monitor_keep_alive_msg = true;
        }
        else if (RTSP_INVALID_MSG_RECEIVED == status_code)
        {
            session_mgr_buffer.action = SESSION_MGR_RTSP_INVALID_MESSAGE;
        }
        else if (RTSP_MSG_FAILURE == status_code)
        {
            session_mgr_buffer.action = SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED;
        }
        else
        {
            session_mgr_buffer.action = SESSION_MGR_RTSP_MSG_TIMEDOUT;
        }
        MIRACASTLOG_TRACE("Msg to SessionMgr Action[%#04X]\n", session_mgr_buffer.action);
        m_session_manager_thread->send_message(&session_mgr_buffer, sizeof(session_mgr_buffer));

        if (SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY != session_mgr_buffer.action)
        {
            continue;
        }

        RTSP_STATUS socket_state;

        while (true == start_monitor_keep_alive_msg)
        {
            memset(&rtsp_message_socket, 0x00, sizeof(rtsp_message_socket));
            socket_state = receive_buffer_timedOut(m_tcpSockfd, rtsp_message_socket, sizeof(rtsp_message_socket));
            if (RTSP_MSG_SUCCESS == socket_state)
            {
                socket_buffer.clear();
                socket_buffer = rtsp_message_socket;
                MIRACASTLOG_TRACE("\n #### RTSP Message [%s] #### \n", socket_buffer.c_str());

                status_code = validate_rtsp_msg_response_back(socket_buffer, RTSP_MSG_POST_M1_M7_XCHANGE);

                MIRACASTLOG_TRACE("[%s] Validate RTSP Msg Action[%#04X] Response[%#04X]\n", __FUNCTION__, rtsp_message_data.action, status_code);
                if ((RTSP_MSG_TEARDOWN_REQUEST == status_code) ||
                    ((RTSP_MSG_SUCCESS != status_code) &&
                     (RTSP_MSG_TEARDOWN_REQUEST != status_code)))
                {
                    session_mgr_buffer.action = SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED;
                    MIRACASTLOG_TRACE("Msg to SessionMgr Action[%#04X]\n", session_mgr_buffer.action);
                    m_session_manager_thread->send_message(&session_mgr_buffer, sizeof(session_mgr_buffer));
                    break;
                }
            }
            else if (RTSP_MSG_FAILURE == socket_state)
            {
                session_mgr_buffer.action = SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED;
                break;
            }

            MIRACASTLOG_TRACE("[%s] Waiting for Event .....\n", __FUNCTION__);
            if (true == m_rtsp_msg_handler_thread->receive_message(&rtsp_message_data, sizeof(rtsp_message_data), 1))
            {
                MIRACASTLOG_TRACE("[%s] Received Action[%#04X]\n", __FUNCTION__, rtsp_message_data.action);
                switch (rtsp_message_data.action)
                {
                case RTSP_SELF_ABORT:
                case RTSP_RESTART:
                {
                    MIRACASTLOG_TRACE("[RTSP_SELF_ABORT/RTSP_RESTART] ACTION Received\n");
                    start_monitor_keep_alive_msg = false;
                }
                break;
                case RTSP_PLAY_FROM_SINK2SRC:
                case RTSP_PAUSE_FROM_SINK2SRC:
                case RTSP_TEARDOWN_FROM_SINK2SRC:
                {
                    MIRACASTLOG_TRACE("[RTSP_PLAY/RTSP_PAUSE/RTSP_TEARDOWN] ACTION Received\n");
                    if ((RTSP_MSG_FAILURE == rtsp_sink2src_request_msg_handling(rtsp_message_data.action) ||
                         (RTSP_TEARDOWN_FROM_SINK2SRC == rtsp_message_data.action)))
                    {
                        start_monitor_keep_alive_msg = false;
                    }
                }
                break;
                default:
                {
                    //
                }
                break;
                }
            }
        }

        MIRACASTLOG_TRACE("[%s] Received Action[%#04X]\n", __FUNCTION__, rtsp_message_data.action);
        if (RTSP_SELF_ABORT == rtsp_message_data.action)
        {
            MIRACASTLOG_TRACE("RTSP_SELF_ABORT ACTION Received\n");
            break;
        }
        else
        {
            session_mgr_buffer.action = SESSION_MGR_RESTART_DISCOVERING;
            MIRACASTLOG_TRACE("Msg to SessionMgr Action[%#04X]\n", session_mgr_buffer.action);
            m_session_manager_thread->send_message(&session_mgr_buffer, sizeof(session_mgr_buffer));
        }
    }
}

void MiracastController::PluginReqHandler_Thread(void *args)
{
    SESSION_MGR_MSG_STRUCT session_mgr_msg_data = {0};
    PLUGIN_REQ_HDLR_MSG_STRUCT plugin_req_hdlr_msg_data = {0};
    bool send_message = false;

    while (true)
    {
        send_message = true;
        memset(&session_mgr_msg_data, 0x00, SESSION_MGR_MSGQ_SIZE);

        MIRACASTLOG_TRACE("[%s] Waiting for Event .....\n", __FUNCTION__);
        m_plugin_req_handler_thread->receive_message(&plugin_req_hdlr_msg_data, sizeof(plugin_req_hdlr_msg_data), MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT);

        MIRACASTLOG_TRACE("[%s] Received Action[%#04X]\n", __FUNCTION__, plugin_req_hdlr_msg_data.action);

        switch (plugin_req_hdlr_msg_data.action)
        {
        case PLUGIN_REQ_HLDR_START_DISCOVER:
        {
            MIRACASTLOG_TRACE("[PLUGIN_REQ_HLDR_START_DISCOVER]\n");
            session_mgr_msg_data.action = SESSION_MGR_START_DISCOVERING;
        }
        break;
        case PLUGIN_REQ_HLDR_STOP_DISCOVER:
        {
            MIRACASTLOG_TRACE("[PLUGIN_REQ_HLDR_STOP_DISCOVER]\n");
            session_mgr_msg_data.action = SESSION_MGR_STOP_DISCOVERING;
        }
        break;
        case PLUGIN_REQ_HLDR_CONNECT_DEVICE_FROM_SESSION_MGR:
        {
            std::string device_name = plugin_req_hdlr_msg_data.buffer_user_data;
            std::string MAC = plugin_req_hdlr_msg_data.action_buffer;

            send_message = true;
            MIRACASTLOG_TRACE("\n################# GO DEVICE[%s - %s] wants to connect: #################\n", device_name.c_str(), MAC.c_str());
            m_notify_handler->onMiracastServiceClientConnectionRequest(MAC, device_name);

#ifdef ENABLE_AUTO_CONNECT
            strcpy(session_mgr_msg_data.event_buffer, MAC.c_str());
            session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_FROM_HANDLER;
#else
                if (true == m_plugin_req_handler_thread->receive_message(&plugin_req_hdlr_msg_data, sizeof(plugin_req_hdlr_msg_data), PLUGIN_REQ_THREAD_CLIENT_CONNECTION_WAITTIME))
                {
                    MIRACASTLOG_TRACE("PluginReqHandler Msg Received [%#04X]\n", plugin_req_hdlr_msg_data.action);
                    if (PLUGIN_REQ_HLDR_CONNECT_DEVICE_ACCEPTED == plugin_req_hdlr_msg_data.action)
                    {
                        strcpy(session_mgr_msg_data.event_buffer, MAC.c_str());
                        session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_FROM_HANDLER;
                    }
                    else if (PLUGIN_REQ_HLDR_CONNECT_DEVICE_REJECTED == plugin_req_hdlr_msg_data.action)
                    {
                        session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_REJECT;
                    }
                    else if (PLUGIN_REQ_HLDR_SHUTDOWN_APP == plugin_req_hdlr_msg_data.action)
                    {
                        session_mgr_msg_data.action = SESSION_MGR_SELF_ABORT;
                    }
                    else if (PLUGIN_REQ_HLDR_STOP_DISCOVER == plugin_req_hdlr_msg_data.action)
                    {
                        session_mgr_msg_data.action = SESSION_MGR_STOP_DISCOVERING;
                    }
                    else
                    {
                        session_mgr_msg_data.action = SESSION_MGR_INVALID_ACTION;
                    }
                }
                else
                {
                    MIRACASTLOG_TRACE("[SESSION_MGR_CONNECT_REQ_TIMEOUT]\n");
                    session_mgr_msg_data.action = SESSION_MGR_CONNECT_REQ_TIMEOUT;
                }
#endif
        }
        break;
        case PLUGIN_REQ_HLDR_SHUTDOWN_APP:
        {
            MIRACASTLOG_TRACE("[PLUGIN_REQ_HLDR_SHUTDOWN_APP]\n");
            session_mgr_msg_data.action = SESSION_MGR_SELF_ABORT;
        }
        break;
        case PLUGIN_REQ_HLDR_TEARDOWN_CONNECTION:
        {
            MIRACASTLOG_TRACE("[PLUGIN_REQ_HLDR_TEARDOWN_CONNECTION]\n");
            session_mgr_msg_data.action = SESSION_MGR_TEARDOWN_REQ_FROM_HANDLER;
        }
        break;
        default:
        {
            //
        }
        break;
        }

        if (true == send_message)
        {
            MIRACASTLOG_INFO("Msg to SessionMgr Action[%#04X]\n", session_mgr_msg_data.action);
            m_session_manager_thread->send_message(&session_mgr_msg_data, SESSION_MGR_MSGQ_SIZE);
            if (PLUGIN_REQ_HLDR_SHUTDOWN_APP == plugin_req_hdlr_msg_data.action)
            {
                break;
            }
        }
    }
}

void MiracastController::SendMessageToPluginReqHandlerThread(size_t action, std::string action_buffer, std::string user_data)
{
    PLUGIN_REQ_HDLR_MSG_STRUCT plugin_req_msg_data = {0};
    bool valid_mesage = true;
    MIRACASTLOG_TRACE("Entering...");
    switch (action)
    {
    case MIRACAST_SERVICE_WFD_START:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_WFD_START]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_START_DISCOVER;
    }
    break;
    case MIRACAST_SERVICE_WFD_STOP:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_WFD_STOP]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_STOP_DISCOVER;
    }
    break;
    case MIRACAST_SERVICE_SHUTDOWN:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_SHUTDOWN]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_SHUTDOWN_APP;
    }
    break;
    case MIRACAST_SERVICE_STOP_CLIENT_CONNECTION:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_STOP_CLIENT_CONNECTION]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_TEARDOWN_CONNECTION;
        memcpy(plugin_req_msg_data.action_buffer, user_data.c_str(), user_data.length());
    }
    break;
#ifndef ENABLE_AUTO_CONNECT
    case MIRACAST_SERVICE_ACCEPT_CLIENT:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_ACCEPT_CLIENT]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_CONNECT_DEVICE_ACCEPTED;
    }
    break;
    case MIRACAST_SERVICE_REJECT_CLIENT:
    {
        MIRACASTLOG_TRACE("[MIRACAST_SERVICE_REJECT_CLIENT]\n");
        plugin_req_msg_data.action = PLUGIN_REQ_HLDR_CONNECT_DEVICE_REJECTED;
    }
    break;
#endif
    default:
    {
        MIRACASTLOG_ERROR("Unknown Action Received [%04X]\n", action);
        valid_mesage = false;
    }
    break;
    }

    MIRACASTLOG_VERBOSE("MiracastController::SendMessageToPluginReqHandler received Action[%#04X]\n", action);

    if (true == valid_mesage)
    {
        MIRACASTLOG_VERBOSE("Msg to PluginReqHdlr Action[%#04X]\n", plugin_req_msg_data.action);
        m_plugin_req_handler_thread->send_message(&plugin_req_msg_data, sizeof(plugin_req_msg_data));
    }
    MIRACASTLOG_TRACE("Exiting...");
}

void PluginReqHandlerCallback(void *args)
{
    MiracastController *miracast_ctrler_obj = (MiracastController *)args;
    miracast_ctrler_obj->PluginReqHandler_Thread(nullptr);
}

void SessionMgrThreadCallback(void *args)
{
    MiracastController *miracast_ctrler_obj = (MiracastController *)args;
    miracast_ctrler_obj->SessionManager_Thread(nullptr);
}

void RTSPMsgHandlerCallback(void *args)
{
    MiracastController *miracast_ctrler_obj = (MiracastController *)args;
    miracast_ctrler_obj->RTSPMessageHandler_Thread(nullptr);
}

#ifdef ENABLE_HDCP2X_SUPPORT
void MiracastController::DumpBuffer(char *buffer, int length)
{
    // Loop through the buffer, printing each byte in hex format
    std::string hex_string;
    for (int i = 0; i < length; i++)
    {
        char hex_byte[3];
        snprintf(hex_byte, sizeof(hex_byte), "%02X", (unsigned char)buffer[i]);
        hex_string += "0x";
        hex_string += hex_byte;
        hex_string += " ";
    }
    MIRACASTLOG_INFO("\n######### DUMP BUFFER[%u] #########\n%s\n###############################\n", length, hex_string.c_str());
}

void MiracastController::HDCPTCPServerHandlerThread(void *args)
{
    char buff[HDCP2X_SOCKET_BUF_MAX];
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        MIRACASTLOG_ERROR("socket creation failed...\n");
    }
    else
        MIRACASTLOG_INFO("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(HDCP2X_PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        MIRACASTLOG_ERROR("socket bind failed...\n");
    }
    else
        MIRACASTLOG_INFO("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        MIRACASTLOG_ERROR("Listen failed...\n");
    }
    else
        MIRACASTLOG_INFO("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0)
    {
        MIRACASTLOG_ERROR("server accept failed...\n");
    }
    else
        MIRACASTLOG_INFO("server accept the client...\n");

    while (true)
    {
        bzero(buff, HDCP2X_SOCKET_BUF_MAX);

        // read the message from client and copy it in buffer
        int n = read(connfd, buff, sizeof(buff));

        if (0 < n)
        {
            DumpBuffer(buff, n);
        }

        bzero(buff, HDCP2X_SOCKET_BUF_MAX);
    }
}

void HDCPHandlerCallback(void *args)
{
    MiracastController *miracast_ctrler_obj = (MiracastController *)args;

    miracast_ctrler_obj->HDCPTCPServerHandlerThread(nullptr);
}
#endif /*ENABLE_HDCP2X_SUPPORT*/