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

#ifndef _MIRACAST_SERVICE_H_
#define _MIRACAST_SERVICE_H_

#include <MiracastServiceError.h>
#include <string>
#include <vector>

using namespace std;
using namespace MIRACAST;

/*@TODO: RENAME and CAPS*/
enum
{
    Stop_Miracast_Service = 1,
    Start_WiFi_Display,
    Stop_WiFi_Display,
    Accept_ConnectDevice_Request,
    Reject_ConnectDevice_Request,
    Stop_Client_Connection
};

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

/**
 * Abstract class for Notification.
 */
using namespace std;
class MiracastServiceNotifier
{
public:
    virtual void onMiracastServiceClientConnectionRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientStopRequest(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientConnectionStarted(string client_mac, string client_name) = 0;
    virtual void onMiracastServiceClientConnectionError(string client_mac, string client_name) = 0;
};

class MiracastPrivate;

class MiracastServiceImplementation
{
public:
    static MiracastServiceImplementation *create(MiracastServiceNotifier *Callback);
    static void Destroy(MiracastServiceImplementation *object);
    void setEnable(std::string is_enabled);
    void acceptClientConnectionRequest(std::string is_accepted);
    bool StopClientConnection(std::string mac_address);

    void setFriendlyName(std::string friendly_name);
    std::string getFriendlyName(void);

    bool enableMiracast(bool flag = false);

    void Shutdown(void);
    // Global APIs
    MiracastError discoverDevices();
    MiracastError selectDevice();
    MiracastError connectDevice(std::string MAC);
    MiracastError startStreaming();

    // APIs to request for device/connection related details
    std::string getConnectedMAC();
    std::vector<DeviceInfo *> getAllPeers();
    bool getConnectionStatus();
    DeviceInfo *getDeviceDetails(std::string MAC);

    // APIs to disconnect
    bool stopStreaming();
    bool disconnectDevice();

private:
    MiracastServiceImplementation(MiracastServiceNotifier *Callback);
    MiracastServiceImplementation();
    MiracastServiceImplementation(MiracastServiceImplementation &);
    ~MiracastServiceImplementation();
    MiracastPrivate *m_impl;
    std::string m_friendly_name;
};

#endif
