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

#ifndef _MIRACAST_P2P_H_
#define _MIRACAST_P2P_H_

#include <string>
#include "wifiSrvMgrIarmIf.h"

using namespace std;
using namespace MIRACAST;

/* Seperate the p2p related to p2p c/h group. */
typedef enum INTERFACE
{
    NON_GLOBAL_INTERFACE = 0,
    GLOBAL_INTERFACE
} P2P_INTERFACE;

enum P2P_EVENTS
{
    EVENT_FOUND = 0,
    EVENT_PROVISION,
    EVENT_STOP,
    EVENT_GO_NEG_REQ,
    EVENT_GO_NEG_SUCCESS,
    EVENT_GO_NEG_FAILURE,
    EVENT_GROUP_STARTED,
    EVENT_FORMATION_SUCCESS,
    EVENT_FORMATION_FAILURE,
    EVENT_DEVICE_LOST,
    EVENT_GROUP_REMOVED,
    EVENT_SHOW_PIN,
    EVENT_ERROR
};

#define MIRACAST_DEFAULT_NAME "Miracast-Generic"

class MiracastP2P
{
    private:
    std::string m_friendly_name;
    std::string m_authType;
    struct wpa_ctrl *wpa_p2p_cmd_ctrl_iface;
    struct wpa_ctrl *wpa_p2p_ctrl_monitor;
    bool stop_p2p_monitor;
    char event_buffer[2048];
    size_t event_buffer_len;
    bool m_isIARMEnabled;
    bool m_isWiFiDisplayParamsEnabled;
    pthread_t p2p_ctrl_monitor_thread_id;

    public:
    /*members for interacting with wpa_supplicant*/
    void Init();
    int p2pInit();
    int p2pUninit();
    void setWiFiDisplayParams();
    void resetWiFiDisplayParams();
    void applyWFDSinkDeviceName();
    int p2pExecute(char *cmd, enum INTERFACE iface, char *status);
    int p2pWpaCtrlSendCmd(char *cmd, struct wpa_ctrl *wpa_p2p_ctrl_iface, char *ret_buf);

};

#endif