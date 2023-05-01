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
#include <MiracastServicePrivate.h>
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
    m_impl = new MiracastPrivate(notifier);
}

MiracastServiceImplementation::MiracastServiceImplementation()
{
}

MiracastServiceImplementation::~MiracastServiceImplementation()
{
    MIRACASTLOG_VERBOSE("Destructor...\n");
    delete m_impl;
}

void MiracastServiceImplementation::setFriendlyName(std::string friendly_name)
{
    m_impl->setFriendlyName(friendly_name);
}

std::string MiracastServiceImplementation::getFriendlyName(void)
{
    return m_impl->getFriendlyName();
}

MiracastError MiracastServiceImplementation::startStreaming()
{
    return m_impl->startStreaming();
}

std::string MiracastServiceImplementation::getConnectedMAC()
{
    return m_impl->getConnectedMAC();
}

std::vector<DeviceInfo *> MiracastServiceImplementation::getAllPeers()
{
    return m_impl->getAllPeers();
}

DeviceInfo *MiracastServiceImplementation::getDeviceDetails(std::string MAC)
{
    return m_impl->getDeviceDetails(MAC);
}

bool MiracastServiceImplementation::getConnectionStatus()
{
    return m_impl->getConnectionStatus();
}

bool MiracastServiceImplementation::stopStreaming()
{
    return m_impl->stopStreaming();
}

void MiracastServiceImplementation::Shutdown(void)
{
    std::string action_buffer;
    std::string user_data;

    m_impl->SendMessageToClientReqHandler(MIRACAST_SERVICE_SHUTDOWN, action_buffer, user_data);
}

void MiracastServiceImplementation::setEnable(bool is_enabled)
{
    /*@TODO : initialized the varriables*/
    std::string action_buffer;
    std::string user_data;
    size_t action;
    bool status = true;

    if ( true == is_enabled)
    {
        action = MIRACAST_SERVICE_WFD_START;
    }
    else
    {
        action = MIRACAST_SERVICE_WFD_STOP;
    }
    /*Check for polimorphism to default value in defination*/
    m_impl->SendMessageToClientReqHandler(action, action_buffer, user_data);
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
    m_impl->SendMessageToClientReqHandler(action, action_buffer, user_data);
}

bool MiracastServiceImplementation::StopClientConnection(std::string mac_address)
{
    std::string action_buffer;

    if (0 != (mac_address.compare(m_impl->getConnectedMAC())))
    {
        return false;
    }
    m_impl->SendMessageToClientReqHandler(MIRACAST_SERVICE_STOP_CLIENT_CONNECTION, action_buffer, mac_address);
    return true;
}