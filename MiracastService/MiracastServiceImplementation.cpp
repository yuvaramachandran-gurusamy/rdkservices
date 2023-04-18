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

#include <MiracastServiceImplementation.h>
#include <MiracastServicePrivate.h>
#include <unistd.h>

using namespace MIRACAST;

MiracastServiceImplementation* MiracastServiceImplementation::create(MiracastServiceNotifier* xrecallback)
{
	std::cout << "MiracastServiceImplementation::create\n";
	MIRACASTLOG_VERBOSE("Service created\n");

	if(! (access( "/opt/disableMiracast", F_OK ) != -1))
	{
		return new MiracastServiceImplementation(xrecallback);
	}
#if 0
	else
	{
		if(xrecallback)
			xrecallback->onMiracastDisabled();
		MIRACASTLOG_VERBOSE("Service cannot be created. Disabled\n");
		return NULL;
	}
#endif
	return NULL;
}

void MiracastServiceImplementation::Destroy( MiracastServiceImplementation* object )
{
	if ( object ){
		delete object;
	}
}

MiracastServiceImplementation::MiracastServiceImplementation(MiracastServiceNotifier* xrecallback)
{
	std::cout << "MiracastServiceImplementation::ctor\n";
	m_impl = new MiracastPrivate(xrecallback);
}

MiracastServiceImplementation::MiracastServiceImplementation()
{
}

MiracastServiceImplementation::~MiracastServiceImplementation()
{
	MIRACASTLOG_INFO("Destructor...\n");
	delete m_impl;
}

MiracastError MiracastServiceImplementation::discoverDevices()
{
	return m_impl->discoverDevices();
}

MiracastError MiracastServiceImplementation::connectDevice(std::string MAC)
{
	return m_impl->connectDevice(MAC);
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

std::vector<DeviceInfo*> MiracastServiceImplementation::getAllPeers()
{
	return m_impl->getAllPeers();
}

DeviceInfo* MiracastServiceImplementation::getDeviceDetails(std::string MAC)
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

bool MiracastServiceImplementation::disconnectDevice()
{
	return m_impl->disconnectDevice();
}

void MiracastServiceImplementation::Shutdown( void )
{
	std::string action_buffer;
	std::string user_data;

	m_impl->SendMessageToClientReqHandler( Stop_Miracast_Service , action_buffer , user_data );
}

void MiracastServiceImplementation::setEnable( std::string is_enabled )
{
	std::string action_buffer;
	std::string user_data;
	size_t action;
	bool status = true;

	std::cout << "MiracastServiceImplementation::setEnable\n";
	if( "true" == is_enabled ){
		action = Start_WiFi_Display;
	}
	else if( "false" == is_enabled ){
		action = Stop_WiFi_Display;
	}
	else{
		status = false;
	}

	if ( true == status ){
		m_impl->SendMessageToClientReqHandler( action , action_buffer , user_data );
	}
}

void MiracastServiceImplementation::acceptClientConnectionRequest( std::string is_accepted )
{
	std::string action_buffer;
	std::string user_data;
	size_t action;

	std::cout << "MiracastServiceImplementation::acceptClientConnectionRequest\n";
	if( "Accept" == is_accepted ){
		MIRACASTLOG_VERBOSE("Client Connection Request accepted\n");
		action = Accept_ConnectDevice_Request;
	}
	else{
		MIRACASTLOG_VERBOSE("Client Connection Request Rejected\n");
		action = Reject_ConnectDevice_Request;
	}
	m_impl->SendMessageToClientReqHandler( action , action_buffer , user_data );
}

bool MiracastServiceImplementation::StopClientConnection( std::string mac_address )
{
	std::string action_buffer;

	if ( 0 != (mac_address.compare(m_impl->getConnectedMAC()))){
		return false;
	}
	m_impl->SendMessageToClientReqHandler( Stop_Client_Connection , action_buffer , mac_address );
	return true;
}