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

#include <MiracastServiceImplementation.h>
#ifdef WFD_V2
#include <MiracastController.h>
#else
#include <MiracastServicePrivate.h>
#endif
#include <unistd.h>

using namespace MIRACAST;

MiracastServiceImplementation *MiracastServiceImplementation::create(MiracastServiceNotifier *notifier)
{
    MIRACASTLOG_VERBOSE("Service created\n");

    return new MiracastServiceImplementation(notifier);
}

void MiracastServiceImplementation::Destroy(MiracastServiceImplementation *object)
{
    if (object)
    {
        delete object;
    }
}

MiracastServiceImplementation::MiracastServiceImplementation(MiracastServiceNotifier *notifier)
{
    MIRACASTLOG_VERBOSE("MiracastServiceImplementation::ctor...\n");
#ifdef WFD_V2
    MIRACASTLOG_VERBOSE("MiracastController based MiracastServiceImplementation\n");
    m_miracast_obj = MiracastController::getInstance(notifier);
#else
    m_impl = new MiracastPrivate(notifier);
#endif
}

MiracastServiceImplementation::MiracastServiceImplementation()
{
}

MiracastServiceImplementation::~MiracastServiceImplementation()
{
    MIRACASTLOG_VERBOSE("Destructor...\n");
#ifdef WFD_V2
    MiracastController::destroyInstance();
    m_miracast_obj = nullptr;
#else
    delete m_impl;
#endif
}

void MiracastServiceImplementation::setFriendlyName(std::string friendly_name)
{
#ifdef WFD_V2
    m_miracast_obj->set_FriendlyName(friendly_name);
#else
    m_impl->setFriendlyName(friendly_name);
#endif
}

std::string MiracastServiceImplementation::getFriendlyName(void)
{
#ifdef WFD_V2
    return m_miracast_obj->get_FriendlyName();
#else
    return m_impl->getFriendlyName();
#endif
}

MiracastError MiracastServiceImplementation::startStreaming()
{
#ifdef WFD_V2
    return m_miracast_obj->start_streaming();
#else
    return m_impl->startStreaming();
#endif
}

std::string MiracastServiceImplementation::getConnectedMAC()
{
#ifdef WFD_V2
    return m_miracast_obj->get_connected_device_mac();
#else
    return m_impl->getConnectedMAC();
#endif
}

std::vector<DeviceInfo *> MiracastServiceImplementation::getAllPeers()
{
#ifdef WFD_V2
    return m_miracast_obj->get_allPeers();
#else
    return m_impl->getAllPeers();
#endif
}

DeviceInfo *MiracastServiceImplementation::getDeviceDetails(std::string MAC)
{
#ifdef WFD_V2
    return m_miracast_obj->get_device_details(MAC);
#else
    return m_impl->getDeviceDetails(MAC);
#endif
}

bool MiracastServiceImplementation::getConnectionStatus()
{
#ifdef WFD_V2
    return m_miracast_obj->get_connection_status();
#else
    return m_impl->getConnectionStatus();
#endif
}

bool MiracastServiceImplementation::stopStreaming()
{
#ifdef WFD_V2
    return m_miracast_obj->stop_streaming();
#else
    return m_impl->stopStreaming();
#endif
}

void MiracastServiceImplementation::Shutdown(void)
{
    std::string action_buffer;
    std::string user_data;
#ifdef WFD_V2
    m_miracast_obj->SendMessageToPluginReqHandlerThread(MIRACAST_SERVICE_SHUTDOWN, action_buffer, user_data);
#else
    m_impl->SendMessageToClientReqHandler(MIRACAST_SERVICE_SHUTDOWN, action_buffer, user_data);
#endif
}

void MiracastServiceImplementation::setEnable(bool is_enabled)
{
    /*@TODO : initialized the varriables*/
    std::string action_buffer;
    std::string user_data;
    size_t action;

    if ( true == is_enabled)
    {
        action = MIRACAST_SERVICE_WFD_START;
    }
    else
    {
        action = MIRACAST_SERVICE_WFD_STOP;
    }
    /*Check for polimorphism to default value in defination*/
#ifdef WFD_V2
    m_miracast_obj->SendMessageToPluginReqHandlerThread(action, action_buffer, user_data);
#else
    m_impl->SendMessageToClientReqHandler(action, action_buffer, user_data);
#endif
}

void MiracastServiceImplementation::acceptClientConnectionRequest(std::string is_accepted)
{
    std::string action_buffer;
    std::string user_data;
    size_t action;

    if ("Accept" == is_accepted)
    {
        MIRACASTLOG_VERBOSE("Client Connection Request accepted\n");
        action = MIRACAST_SERVICE_ACCEPT_CLIENT;
    }
    else
    {
        MIRACASTLOG_VERBOSE("Client Connection Request Rejected\n");
        action = MIRACAST_SERVICE_REJECT_CLIENT;
    }
#ifdef WFD_V2
    m_miracast_obj->SendMessageToPluginReqHandlerThread(action, action_buffer, user_data);
#else
    m_impl->SendMessageToClientReqHandler(action, action_buffer, user_data);
#endif
}

bool MiracastServiceImplementation::StopClientConnection(std::string mac_address)
{
    std::string action_buffer;
#ifdef WFD_V2
    if (0 != (mac_address.compare(m_miracast_obj->get_connected_device_mac())))
    {
        return false;
    }
    m_miracast_obj->SendMessageToPluginReqHandlerThread(MIRACAST_SERVICE_STOP_CLIENT_CONNECTION, action_buffer, mac_address);
#else
    if (0 != (mac_address.compare(m_impl->getConnectedMAC())))
    {
        return false;
    }
    m_impl->SendMessageToClientReqHandler(MIRACAST_SERVICE_STOP_CLIENT_CONNECTION, action_buffer, mac_address);
#endif
    return true;
}