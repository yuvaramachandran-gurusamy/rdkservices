/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#include <algorithm>
#include <regex>
#include "rdk/iarmmgrs-hal/pwrMgr.h"
#include "MiracastPlayer.h"
#include <UtilsJsonRpc.h>
#include "UtilsIarm.h"

const short WPEFramework::Plugin::MiracastPlayer::API_VERSION_NUMBER_MAJOR = 1;
const short WPEFramework::Plugin::MiracastPlayer::API_VERSION_NUMBER_MINOR = 0;
const string WPEFramework::Plugin::MiracastPlayer::SERVICE_NAME = "org.rdk.MiracastPlayer";
using namespace std;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

#define SERVER_DETAILS  "127.0.0.1:9998"
#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN".1"
#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 2000

#define EVT_ON_CLIENT_CONNECTION_REQUEST "onClientConnectionRequest"
#define EVT_ON_CLIENT_STOP_REQUEST "onClientStopRequest"
#define EVT_ON_CLIENT_CONNECTION_STARTED "onClientConnectionStarted"
#define EVT_ON_CLIENT_CONNECTION_ERROR "onClientConnectionError"

namespace WPEFramework
{
	namespace
	{
		static Plugin::Metadata<Plugin::MiracastPlayer> metadata(
			// Version (Major, Minor, Patch)
			API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
			// Preconditions
			{},
			// Terminations
			{},
			// Controls
			{});
	}

	namespace Plugin
	{
		SERVICE_REGISTRATION(MiracastPlayer, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

		MiracastPlayer *MiracastPlayer::_instance = nullptr;
		MiracastRTSPMsg *MiracastPlayer::m_miracast_rtsp_obj = nullptr;

		MiracastPlayer::MiracastPlayer()
			: PluginHost::JSONRPC(),
			  m_isServiceInitialized(false),
			  m_isServiceEnabled(false)
		{
			LOGINFO("Entering..!!!");
			MiracastPlayer::_instance = this;

			MIRACAST::logger_init(STR(MODULE_NAME));
			LOGINFO("Exiting..!!!");
		}

		MiracastPlayer::~MiracastPlayer()
		{
			LOGINFO("Entering..!!!");
			if (nullptr != m_SystemPluginObj)
			{
				delete m_SystemPluginObj;
				m_SystemPluginObj = nullptr;
			}
			MIRACAST::logger_deinit();
			LOGINFO("Exiting..!!!");
		}

		// Thunder plugins communication
        void MiracastPlayer::getSystemPlugin()
        {
			LOGINFO("Entering..!!!");

            if(nullptr == m_SystemPluginObj)
            {
				string token;
                // TODO: use interfaces and remove token
                auto security = m_CurrentService->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
                if (nullptr != security)
				{
                    string payload = "http://localhost";
                    if (security->CreateToken( static_cast<uint16_t>(payload.length()),
											reinterpret_cast<const uint8_t*>(payload.c_str()),
                            				token) == Core::ERROR_NONE)
					{
						LOGINFO("got security token\n");
                    }
					else
					{
						LOGERR("failed to get security token\n");
                    }
                    security->Release();
                }
				else
				{
					LOGERR("No security agent\n");
                }

                string query = "token=" + token;
                Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
                m_SystemPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(SYSTEM_CALLSIGN_VER), (_T("MiracastPlayer")), false, query);
				if (nullptr == m_SystemPluginObj)
				{
					LOGERR("JSONRPC: %s: initialization failed", SYSTEM_CALLSIGN_VER);
				}
				else
				{
					LOGINFO("JSONRPC: %s: initialization ok", SYSTEM_CALLSIGN_VER);
				}
            }
			LOGINFO("Exiting..!!!");
        }

		const string MiracastPlayer::Initialize(PluginHost::IShell *service)
		{
			string msg;
			LOGINFO("Entering..!!!");
			if (!m_isServiceInitialized)
			{
				MiracastError ret_code = MIRACAST_OK;
				m_miracast_rtsp_obj = MiracastRTSPMsg::getInstance(ret_code,this);
				if ( nullptr != m_miracast_rtsp_obj ){
					m_CurrentService = service;
					getSystemPlugin();
					m_isServiceInitialized = true;
				}
				else{
					switch (ret_code){
						case MIRACAST_INVALID_P2P_CTRL_IFACE:
						{
							msg = "Invalid P2P Ctrl iface configured";
						}
						break;
						case MIRACAST_CONTROLLER_INIT_FAILED:
						{
							msg = "Controller Init Failed";
						}
						break;
						case MIRACAST_P2P_INIT_FAILED:
						{
							msg = "P2P Init Failed";
						}
						break;
						case MIRACAST_RTSP_INIT_FAILED:
						{
							msg = "RTSP msg handler Init Failed";
						}
						break;
						default:
						{
							msg = "Unknown Error:Failed to obtain MiracastRTSPMsg Object";
						}
						break;
					}
				}
			}

			// On success return empty, to indicate there is no error text.
			return msg;
		}

		void MiracastPlayer::Deinitialize(PluginHost::IShell * /* service */)
		{
			MiracastPlayer::_instance = nullptr;
			LOGINFO("Entering..!!!");

			if (m_isServiceInitialized)
			{
				MiracastRTSPMsg::destroyInstance();
				m_CurrentService = nullptr;
				m_miracast_rtsp_obj = nullptr;
				m_isServiceInitialized = false;
				m_isServiceEnabled = false;
				LOGINFO("Done..!!!");
			}
			LOGINFO("Exiting..!!!");
		}

		string MiracastPlayer::Information() const
		{
			return (string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
		}

		void MiracastPlayer::onMiracastPlayerClientConnectionRequest(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_REQUEST, params);
		}

		void MiracastPlayer::onMiracastPlayerClientStopRequest(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_STOP_REQUEST, params);
		}

		void MiracastPlayer::onMiracastPlayerClientConnectionStarted(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_STARTED, params);
		}

		void MiracastPlayer::onMiracastPlayerClientConnectionError(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_ERROR, params);
		}
	} // namespace Plugin
} // namespace WPEFramework