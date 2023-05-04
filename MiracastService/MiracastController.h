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

#ifndef _MIRACAST_CONTROLLER_H_
#define _MIRACAST_CONTROLLER_H_

#include <string>
#include <sstream>
#include <fcntl.h>
#include <algorithm>
#include <vector>
#include <glib.h>
#include <semaphore.h>
#include "libIBus.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fstream>
#include "MiracastServiceImplementation.h"
#include "MiracastP2P.h"
#include "MiracastLogger.h"
#include "MiracastRtspMsg.h"
#include "wifiSrvMgrIarmIf.h"

using namespace std;
using namespace MIRACAST;

#define REMOVE_SPACES(x) x.erase(std::remove(x.begin(), x.end(), ' '), x.end())
#define REMOVE_R(x) x.erase(std::remove(x.begin(), x.end(), '\r'), x.end())
#define REMOVE_N(x) x.erase(std::remove(x.begin(), x.end(), '\n'), x.end())

#define MAX_EPOLL_EVENTS 64
#define SPACE " "

#define ENABLE_NON_BLOCKING /* @TODO: Remove and keep non blocking as default.*/
#define ONE_SECOND_PER_MILLISEC (1000)
#define SOCKET_WAIT_TIMEOUT_IN_MILLISEC (30 * ONE_SECOND_PER_MILLISEC)

#define SESSION_MGR_NAME ("MIRA_SESSION_MGR") /*@TODO: Change name to Msg_handler_Thread*/
#define SESSION_MGR_STACK (256 * 1024)
#define SESSION_MGR_MSG_COUNT (5)
#define SESSION_MGR_MSGQ_SIZE (sizeof(SESSION_MGR_MSG_STRUCT))

#define PLUGIN_REQ_HANDLER_NAME ("MIRA_PLUGIN_REQ_HLDR") /*@TODO: Change name to THUNDER_REQ_HANDLER_NAME */
#define PLUGIN_REQ_HANDLER_STACK (256 * 1024)

#define MIRACAST_THREAD_RECV_MSG_INDEFINITE_WAIT (-1)
#define PLUGIN_REQ_THREAD_CLIENT_CONNECTION_WAITTIME (30)

/*@TODO: Change name to relavant to Msg_handler_Thread*/
typedef enum session_manager_actions_e
{
    SESSION_MGR_START_DISCOVERING = 0x01,
    SESSION_MGR_STOP_DISCOVERING,
    SESSION_MGR_RESTART_DISCOVERING,
    SESSION_MGR_GO_DEVICE_FOUND,
    SESSION_MGR_GO_DEVICE_LOST,
    SESSION_MGR_GO_DEVICE_PROVISION,
    SESSION_MGR_GO_STOP_FIND,
    SESSION_MGR_GO_NEG_REQUEST,
    SESSION_MGR_GO_NEG_SUCCESS,
    SESSION_MGR_GO_NEG_FAILURE,
    SESSION_MGR_CONNECT_REQ_FROM_HANDLER,
    SESSION_MGR_CONNECT_REQ_REJECT,
    SESSION_MGR_CONNECT_REQ_TIMEOUT,
    SESSION_MGR_TEARDOWN_REQ_FROM_HANDLER,
    SESSION_MGR_GO_GROUP_STARTED,
    SESSION_MGR_GO_GROUP_REMOVED,
    SESSION_MGR_GO_GROUP_FORMATION_SUCCESS,
    SESSION_MGR_GO_GROUP_FORMATION_FAILURE,
    SESSION_MGR_START_STREAMING,
    SESSION_MGR_PAUSE_STREAMING,
    SESSION_MGR_STOP_STREAMING,
    SESSION_MGR_RTSP_MSG_RECEIVED_PROPERLY,
    SESSION_MGR_RTSP_MSG_TIMEDOUT,
    SESSION_MGR_RTSP_INVALID_MESSAGE,
    SESSION_MGR_RTSP_SEND_REQ_RESP_FAILED,
    SESSION_MGR_RTSP_TEARDOWN_REQ_RECEIVED,
    SESSION_MGR_GO_EVENT_ERROR,
    SESSION_MGR_GO_UNKNOWN_EVENT,
    SESSION_MGR_SELF_ABORT,
    SESSION_MGR_INVALID_ACTION
} SESSION_MANAGER_ACTIONS;

/*@TODO: Move to same state/action enum group with name prefix*/
typedef enum plugin_req_handler_actions_e
{
    PLUGIN_REQ_HLDR_START_DISCOVER = 0x01,
    PLUGIN_REQ_HLDR_STOP_DISCOVER,
    PLUGIN_REQ_HLDR_CONNECT_DEVICE_FROM_SESSION_MGR,
    PLUGIN_REQ_HLDR_CONNECT_DEVICE_ACCEPTED,
    PLUGIN_REQ_HLDR_CONNECT_DEVICE_REJECTED,
    PLUGIN_REQ_HLDR_TEARDOWN_CONNECTION,
    PLUGIN_REQ_HLDR_SHUTDOWN_APP
} PLUGIN_REQ_HANDLER_ACTIONS;

typedef struct group_info
{
    std::string interface;
    bool isGO;
    std::string SSID;
    std::string goDevAddr;
    std::string ipAddr;
    std::string ipMask;
    std::string goIPAddr;
    std::string localIPAddr;
} GroupInfo;

/*
typefed enum Msg_Type_e {
    P2P_MSG,
    RTSP_MSG
} eMsg_Type
//typedef struct con_mgr_str
//{
    char msg_buffer[2048];
    MSG_HDLR_ACTIONS action;
    eMsg_Type msg_type
} SESSION_MGR_MSG_STRUCT;
*/
/* Move to top before using this.*/
/*@TODO: Change name to p2p_event_msg*/
typedef struct session_mgr_msg
{
    char event_buffer[2048];
    SESSION_MANAGER_ACTIONS action;
} SESSION_MGR_MSG_STRUCT;

/*@TODO: Change name to rtsp_msg_hdler*/
typedef struct rtsp_hldr_msg
{
    RTSP_MSG_HANDLER_ACTIONS action;
    size_t userdata;
} RTSP_HLDR_MSG_STRUCT;

/* Related to plugin method, rename*/
typedef struct plugin_req_hldr_msg
{
    char action_buffer[32];
    char buffer_user_data[32];
    PLUGIN_REQ_HANDLER_ACTIONS action;
} PLUGIN_REQ_HDLR_MSG_STRUCT;

/*@TODO: Naming of memeber varibale (m_ )*/
class MiracastThread
{
public:
    MiracastThread();
    MiracastThread(std::string thread_name, size_t stack_size, size_t msg_size, size_t queue_depth, void (*callback)(void *), void *user_data);
    ~MiracastThread();
    void start(void);
    void send_message(void *message, size_t msg_size);
    int8_t receive_message(void *message, size_t msg_size, int sem_wait_timedout);
    void *thread_user_data;

private:
    pthread_t thread_;
    pthread_attr_t attr_;
    sem_t sem_object;
    GAsyncQueue *g_queue;
    size_t thread_stacksize;
    size_t thread_message_size;
    size_t thread_message_count;
    void (*thread_callback)(void *);
};

class MiracastController
{
public:
    static MiracastController *getInstance(MiracastServiceNotifier *notifier = nullptr);
    static void destroyInstance();

    void event_handler(P2P_EVENTS eventId, void *data, size_t len, bool isIARMEnabled = false);

    MiracastError discover_devices();
    MiracastError connect_device(std::string mac);
    MiracastError start_streaming();

    std::string get_localIp();
    std::string get_wfd_streaming_port_number();
    std::string get_connected_device_mac();
    std::vector<DeviceInfo *> get_allPeers();

    bool get_connection_status();
    DeviceInfo *get_device_details(std::string mac);

    MiracastError stop_streaming();
    MiracastError disconnect_device();
    /*@TODO: Change to send_msg_thunder_msg_hdler_thread()*/
    void SendMessageToPluginReqHandlerThread(size_t action, std::string action_buffer, std::string user_data);

    // @TODO: Session Manager, change controller_mgs_hdlr_thread()
    void SessionManager_Thread(void *args);
    // @TODO: RTSP Message Handler
    void RTSPMessageHandler_Thread(void *args);
    // @TODO: Plugin Request Handler
    void PluginReqHandler_Thread(void *args);
    // void HDCPTCPServerHandlerThread(void *args);
    // void DumpBuffer(char *buffer, int length);

    RTSP_STATUS receive_buffer_timedOut(int sockfd, void *buffer, size_t buffer_len);
    RTSP_STATUS send_rstp_msg(int sockfd, std::string rtsp_response_buffer);
    MiracastError stop_discover_devices();
    MiracastError set_WFDParameters(void);
    void restart_session(void);
    void stop_session(void);
    std::string get_device_name(std::string mac);
    MiracastError set_FriendlyName(std::string friendly_name);
    std::string get_FriendlyName(void);

private:
    static MiracastController *m_miracast_ctrl_obj;
    MiracastController();
    virtual ~MiracastController();
    MiracastController &operator=(const MiracastController &) = delete;
    MiracastController(const MiracastController &) = delete;

    std::string parse_p2p_event_data(const char *tmpBuff, const char *lookup_data);
    std::string start_DHCPClient(std::string interface, std::string &default_gw_ip_addr);
    MiracastError initiate_TCP(std::string go_ip);
    MiracastError connect_Sink();
    void create_ControllerFramework(void);
    void destroy_ControllerFramework(void);

    void set_localIp(std::string ipAddr);

    MiracastServiceNotifier *m_notify_handler;
    std::string m_localIp;
    vector<DeviceInfo *> m_deviceInfoList;
    GroupInfo *m_groupInfo;
    bool m_connectionStatus;
    int m_tcpSockfd;
    // int m_hdcptcpSockfd;

    /*members for interacting with wpa_supplicant*/
    MiracastP2P *m_p2p_ctrl_obj;

    MiracastRTSPMsg *m_rtsp_msg;
    MiracastThread *m_plugin_req_handler_thread;
    MiracastThread *m_session_manager_thread;
    MiracastThread *m_rtsp_msg_handler_thread;
    MiracastThread *m_hdcp_handler_thread;

    bool wait_data_timeout(int m_Sockfd, unsigned ms);

    RTSP_STATUS validate_rtsp_msg_response_back(std::string rtsp_msg_buffer, RTSP_MSG_HANDLER_ACTIONS action_id);
    RTSP_STATUS validate_rtsp_m1_msg_m2_send_request(std::string rtsp_m1_msg_buffer);
    RTSP_STATUS validate_rtsp_m2_request_ack(std::string rtsp_m1_response_ack_buffer);
    RTSP_STATUS validate_rtsp_m3_response_back(std::string rtsp_m3_msg_buffer);
    RTSP_STATUS validate_rtsp_m4_response_back(std::string rtsp_m4_msg_buffer);
    RTSP_STATUS validate_rtsp_m5_msg_m6_send_request(std::string rtsp_m5_msg_buffer);
    RTSP_STATUS validate_rtsp_m6_ack_m7_send_request(std::string rtsp_m6_ack_buffer);
    RTSP_STATUS validate_rtsp_m7_request_ack(std::string rtsp_m7_ack_buffer);
    RTSP_STATUS validate_rtsp_post_m1_m7_xchange(std::string rtsp_post_m1_m7_xchange_buffer);
    RTSP_STATUS rtsp_sink2src_request_msg_handling(RTSP_MSG_HANDLER_ACTIONS action_id);
    SESSION_MANAGER_ACTIONS convertP2PtoSessionActions(P2P_EVENTS eventId);
};

#endif