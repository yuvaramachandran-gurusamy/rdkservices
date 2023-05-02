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

#ifndef _MIRACAST_CORE_H_
#define _MIRACAST_CORE_H_

#include <string>
#include "MiracastP2P.h"
#include "MiracastRtspMsg.h"

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


class MiracastCore
{
public:
    MiracastCore();
    ~MiracastCore();
    MiracastCore(MiracastServiceNotifier *notifier);
 
    MiracastError discover_devices();
    MiracastError connect_device(std::string mac);
    MiracastError start_streaming();

    std::string get_localIp();
    std::string get_wfd_streaming_port_number();
    std::string get_connected_device_mac();
    std::vector<DeviceInfo *> get_allPeers();

    bool get_connection_status();
    DeviceInfo *get_device_details(std::string mac);
    MiracastError stop_Streaming();
    bool disconnect_Device();
    
    MiracastError stop_discover_devices();
    void restart_session(void);
    void StopSession(void);
    std::string get_device_name(std::string mac);

    void evtHandler(enum P2P_EVENTS eventId, void *data, size_t len);

    /*Rename*/
    void SendMessageToClientReqHandlerThread(size_t action, std::string action_buffer, std::string user_data);
    
    // Session Manager
    void SessionManager_Thread(void *args);
    // RTSP Message Handler
    void RTSPMessageHandler_Thread(void *args);

    // Client Request Handler
    void ClientRequestHandler_Thread(void *args);
    //void HDCPTCPServerHandlerThread(void *args);
    //void DumpBuffer(char *buffer, int length);

    RTSP_CTRL_SOCKET_STATES receive_buffer_timedOut(int sockfd, void *buffer, size_t buffer_len);
    
    bool send_buffer_timedOut(int sockfd, std::string rtsp_response_buffer);
 
private:
    MiracastError executeCommand(std::string command, int interface, std::string &retBuffer);
    std::string storeData(const char *tmpBuff, const char *lookup_data);
    std::string startDHCPClient(std::string interface, std::string &default_gw_ip_addr);
    bool initiateTCP(std::string goIP);
    bool connectSink();
    void wfdInit(MiracastServiceNotifier *notifier);

    MiracastP2P *m_miracastP2pObj;
    std::string m_friendly_name;
    MiracastServiceNotifier *m_eventCallback;
    vector<DeviceInfo *> m_deviceInfo;
    GroupInfo *m_groupInfo;
    std::string m_authType;
    std::string m_iface;
    bool m_connectionStatus;
    int m_tcpSockfd;
    int m_hdcptcpSockfd;

    MiracastRTSPMessages *m_rtsp_msg;
    MiracastThread *m_client_req_handler_thread;
    MiracastThread *m_session_manager_thread;
    MiracastThread *m_rtsp_msg_handler_thread;
    MiracastThread *m_hdcp_handler_thread;

    bool wait_data_timeout(int m_Sockfd, unsigned ms);

    RTSP_CODE validate_rtsp_msg_response_back(std::string rtsp_msg_buffer, RTSP_MSG_HANDLER_ACTIONS action_id);
    RTSP_CODE validate_rtsp_m1_msg_m2_send_request(std::string rtsp_m1_msg_buffer);
    RTSP_CODE validate_rtsp_m2_request_ack(std::string rtsp_m1_response_ack_buffer);
    RTSP_CODE validate_rtsp_m3_response_back(std::string rtsp_m3_msg_buffer);
    RTSP_CODE validate_rtsp_m4_response_back(std::string rtsp_m4_msg_buffer);
    RTSP_CODE validate_rtsp_m5_msg_m6_send_request(std::string rtsp_m5_msg_buffer);
    RTSP_CODE validate_rtsp_m6_ack_m7_send_request(std::string rtsp_m6_ack_buffer);
    RTSP_CODE validate_rtsp_m7_request_ack(std::string rtsp_m7_ack_buffer);
    RTSP_CODE validate_rtsp_post_m1_m7_xchange(std::string rtsp_post_m1_m7_xchange_buffer);
    RTSP_CODE rtsp_sink2src_request_msg_handling(RTSP_MSG_HANDLER_ACTIONS action_id);
    SESSION_MANAGER_ACTIONS convertP2PtoSessionActions(enum P2P_EVENTS eventId);

};

#endif