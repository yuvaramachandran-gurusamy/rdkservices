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

#ifndef _MIRACAST_COMMON_H_
#define _MIRACAST_COMMON_H_

#include <string>
#include <regex>
#include <sstream>
#include <algorithm>
#include <vector>
#include <glib.h>
#include <semaphore.h>
#include <MiracastServiceError.h>
#include <MiracastLogger.h>

using namespace std;
using namespace MIRACAST;

#define REMOVE_SPACES(x) x.erase(std::remove(x.begin(), x.end(), ' '), x.end())
#define REMOVE_R(x) x.erase(std::remove(x.begin(), x.end(), '\r'), x.end())
#define REMOVE_N(x) x.erase(std::remove(x.begin(), x.end(), '\n'), x.end())

#define SPACE_CHAR " "

#define ONE_SECOND_IN_MILLISEC (1000)

#define THREAD_RECV_MSG_INDEFINITE_WAIT (-1)

#define WPA_SUP_DFLT_CTRL_PATH "/var/run/wpa_supplicant/p2p-dev-wlan0"

typedef enum controller_framework_states_e
{
    THUNDER_REQ_HLDR_START_DISCOVER = 0x00000001,
    THUNDER_REQ_HLDR_STOP_DISCOVER = 0x00000002,
    THUNDER_REQ_HLDR_CONNECT_DEVICE_FROM_CONTROLLER = 0x00000003,
    THUNDER_REQ_HLDR_CONNECT_DEVICE_ACCEPTED = 0x00000004,
    THUNDER_REQ_HLDR_CONNECT_DEVICE_REJECTED = 0x00000005,
    THUNDER_REQ_HLDR_TEARDOWN_CONNECTION = 0x00000006,
    THUNDER_REQ_HLDR_SHUTDOWN_APP = 0x00000007,
    CONTROLLER_START_DISCOVERING = 0x0000FF00,
    CONTROLLER_STOP_DISCOVERING = 0x0000FF01,
    CONTROLLER_START_STREAMING = 0x0000FF02,
    CONTROLLER_PAUSE_STREAMING = 0x0000FF03,
    CONTROLLER_STOP_STREAMING = 0x0000FF04,
    CONTROLLER_CONNECT_REQ_FROM_THUNDER = 0x0000FF05,
    CONTROLLER_CONNECT_REQ_REJECT = 0x0000FF06,
    CONTROLLER_CONNECT_REQ_TIMEOUT = 0x0000FF07,
    CONTROLLER_TEARDOWN_REQ_FROM_THUNDER = 0x0000FF08,
    CONTROLLER_GO_DEVICE_FOUND = 0x0000FF09,
    CONTROLLER_GO_DEVICE_LOST = 0x0000FF0A,
    CONTROLLER_GO_DEVICE_PROVISION = 0x0000FF0B,
    CONTROLLER_GO_STOP_FIND = 0x0000FF0C,
    CONTROLLER_GO_NEG_REQUEST = 0x0000FF0D,
    CONTROLLER_GO_NEG_SUCCESS = 0x0000FF0E,
    CONTROLLER_GO_NEG_FAILURE = 0x0000FF0F,
    CONTROLLER_GO_GROUP_STARTED = 0x0000FF10,
    CONTROLLER_GO_GROUP_REMOVED = 0x0000FF11,
    CONTROLLER_GO_GROUP_FORMATION_SUCCESS = 0x0000FF12,
    CONTROLLER_GO_GROUP_FORMATION_FAILURE = 0x0000FF13,
    CONTROLLER_GO_EVENT_ERROR = 0x0000FF14,
    CONTROLLER_GO_UNKNOWN_EVENT = 0x0000FF15,
    CONTROLLER_RTSP_MSG_RECEIVED_PROPERLY = 0x0000FF16,
    CONTROLLER_RTSP_MSG_TIMEDOUT = 0x0000FF17,
    CONTROLLER_RTSP_INVALID_MESSAGE = 0x0000FF18,
    CONTROLLER_RTSP_SEND_REQ_RESP_FAILED = 0x0000FF19,
    CONTROLLER_RTSP_TEARDOWN_REQ_RECEIVED = 0x0000FF1A,
    CONTROLLER_RTSP_RESTART_DISCOVERING = 0x0000FF1B,
    CONTROLLER_SELF_ABORT = 0x0000FF1C,
    CONTROLLER_INVALID_STATE = 0x0000FF1D,
    RTSP_M1_REQUEST_RECEIVED = 0x000FF0000,
    RTSP_M2_REQUEST_ACK = 0x000FF0001,
    RTSP_M3_REQUEST_RECEIVED = 0x000FF0002,
    RTSP_M4_REQUEST_RECEIVED = 0x000FF0003,
    RTSP_M5_REQUEST_RECEIVED = 0x000FF0004,
    RTSP_M6_REQUEST_ACK = 0x000FF0005,
    RTSP_M7_REQUEST_ACK = 0x000FF0006,
    RTSP_MSG_POST_M1_M7_XCHANGE = 0x000FF0007,
    RTSP_START_RECEIVE_MSGS = 0x000FF0008,
    RTSP_TEARDOWN_FROM_SINK2SRC = 0x000FF0009,
    RTSP_RESTART = 0x000FF000A,
    RTSP_PAUSE_FROM_SINK2SRC = 0x000FF000B,
    RTSP_PLAY_FROM_SINK2SRC = 0x000FF000C,
    RTSP_SELF_ABORT = 0x000FF000D,
    RTSP_INVALID_ACTION
}eCONTROLLER_FW_STATES;

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

typedef enum msg_type_e
{
    P2P_MSG = 0x01,
    RTSP_MSG,
    CONTRLR_FW_MSG
}eMSG_TYPE;

typedef enum miracast_service_states_e
{
    MIRACAST_SERVICE_SHUTDOWN = 0x01,
    MIRACAST_SERVICE_WFD_START,
    MIRACAST_SERVICE_WFD_STOP,
    MIRACAST_SERVICE_ACCEPT_CLIENT,
    MIRACAST_SERVICE_REJECT_CLIENT,
    MIRACAST_SERVICE_STOP_CLIENT_CONNECTION
} MIRACAST_SERVICE_STATES;

enum DEVICEROLE
{
    DEVICEROLE_SOURCE = 0,
    DEVICEROLE_PRIMARY_SINK,
    DEVICEROLE_SECONDARY_SINK,
    DEVICEROLE_DUAL_ROLE
};

typedef struct d_info
{
    string deviceMAC;
    string deviceType;
    string modelName;
    bool isCPSupported;
    enum DEVICEROLE deviceRole;
} DeviceInfo;

typedef struct controller_msgq_st
{
    char msg_buffer[2048];
    eCONTROLLER_FW_STATES state;
    eMSG_TYPE msg_type;
} CONTROLLER_MSGQ_STRUCT;

typedef struct rtsp_hldr_msgq_st
{
    eCONTROLLER_FW_STATES state;
    size_t userdata;
} RTSP_HLDR_MSGQ_STRUCT;

typedef struct thunder_req_hldr_msg_st
{
    char msg_buffer[32];
    char buffer_user_data[32];
    eCONTROLLER_FW_STATES state;
}THUNDER_REQ_HDLR_MSGQ_STRUCT;

#define CONTROLLER_THREAD_NAME ("CONTROL_MSG_HANDLER")
#define CONTROLLER_THREAD_STACK (256 * 1024)
#define CONTROLLER_MSGQ_COUNT (5)
#define CONTROLLER_MSGQ_SIZE (sizeof(CONTROLLER_MSGQ_STRUCT))

#define THUNDER_REQ_HANDLER_THREAD_NAME ("THUNDER_REQ_HLDR")
#define THUNDER_REQ_HANDLER_THREAD_STACK (256 * 1024)
#define THUNDER_REQ_HANDLER_MSGQ_COUNT (1)
#define THUNDER_REQ_HANDLER_MSGQ_SIZE (sizeof(THUNDER_REQ_HDLR_MSGQ_STRUCT))

#define RTSP_HANDLER_THREAD_NAME ("RTSP_MSG_HLDR")
#define RTSP_HANDLER_THREAD_STACK (256 * 1024)
#define RTSP_HANDLER_MSG_COUNT (2)
#define RTSP_HANDLER_MSGQ_SIZE (sizeof(RTSP_HLDR_MSGQ_STRUCT))

#ifdef ENABLE_TEST_NOTIFIER
typedef enum test_notifier_states_e{
    TEST_NOTIFIER_INVALID_STATE = 0x00,
    TEST_NOTIFIER_CLIENT_CONNECTION_REQUESTED,
    TEST_NOTIFIER_CLIENT_STOP_REQUESTED,
    TEST_NOTIFIER_CLIENT_CONNECTION_STARTED,
    TEST_NOTIFIER_CLIENT_CONNECTION_ERROR,
    TEST_NOTIFIER_SHUTDOWN
}
TEST_NOTIFIER_STATES;

#define TEST_NOTIFIER_THREAD_NAME ("TEST_NOTIFIER")
#define TEST_NOTIFIER_THREAD_STACK (20 * 1024)
#define TEST_NOTIFIER_MSG_COUNT (1)
#define TEST_NOTIFIER_MSGQ_SIZE (sizeof(TEST_NOTIFIER_STATES))

#endif /*ENABLE_TEST_NOTIFIER*/

/**
 * Abstract class for MiracastService Notification.
 */
class MiracastServiceNotifier
{
public:
    virtual void onMiracastServiceClientConnectionRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientStopRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientConnectionStarted(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientConnectionError(string client_mac, string client_name) = 0;
};

/**
 * Abstract class for MiracastPlayer Notification.
 */
class MiracastPlayerNotifier
{
public:
    virtual void onMiracastPlayerClientConnectionRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastPlayerClientStopRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastPlayerClientConnectionStarted(string client_mac, string client_name) = 0;
    virtual void onMiracastPlayerClientConnectionError(string client_mac, string client_name) = 0;
};

class MiracastThread
{
public:
    MiracastThread();
    MiracastThread(std::string thread_name, size_t stack_size, size_t msg_size, size_t queue_depth, void (*callback)(void *), void *user_data);
    ~MiracastThread();
    MiracastError start(void);
    void send_message(void *message, size_t msg_size);
    int8_t receive_message(void *message, size_t msg_size, int sem_wait_timedout);

private:
    std::string m_thread_name;
    pthread_t m_pthread_id;
    pthread_attr_t m_pthread_attr;
    sem_t m_empty_msgq_sem_obj;
    GAsyncQueue *m_g_queue;
    size_t m_thread_stacksize;
    size_t m_thread_message_size;
    size_t m_thread_message_count;
    void (*m_thread_callback)(void *);    
    void *m_thread_user_data;
};
#endif