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
#include "MiracastService.h"
#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"

const short WPEFramework::Plugin::MiracastService::API_VERSION_NUMBER_MAJOR = 1;
const short WPEFramework::Plugin::MiracastService::API_VERSION_NUMBER_MINOR = 0;
const string WPEFramework::Plugin::MiracastService::SERVICE_NAME = "org.rdk.MiracastService";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_SET_ENABLE = "setEnable";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_GET_ENABLE = "getEnable";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_CLIENT_CONNECT = "acceptClientConnection";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_STOP_CLIENT_CONNECT = "stopClientConnection";

using namespace std;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

#define SERVER_DETAILS  "127.0.0.1:9998"
#define XCAST_CALLSIGN "org.rdk.Xcast"
#define XCAST_CALLSIGN_VER XCAST_CALLSIGN".1"
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
		static Plugin::Metadata<Plugin::MiracastService> metadata(
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
		SERVICE_REGISTRATION(MiracastService, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

		MiracastService *MiracastService::_instance = nullptr;
		MiracastServiceImplementation *MiracastService::m_miracast_service_impl = nullptr;

		MiracastService::MiracastService()
			: PluginHost::JSONRPC(),
			  m_isServiceInitialized(false),
			  m_isServiceEnabled(false)
		{
			LOGINFO("Entering..!!!");
			MiracastService::_instance = this;

			Register(METHOD_MIRACAST_SET_ENABLE, &MiracastService::setEnable, this);
			Register(METHOD_MIRACAST_GET_ENABLE, &MiracastService::getEnable, this);
			Register(METHOD_MIRACAST_STOP_CLIENT_CONNECT, &MiracastService::stopClientConnection, this);
			Register(METHOD_MIRACAST_CLIENT_CONNECT, &MiracastService::acceptClientConnection, this);
		}

		MiracastService::~MiracastService()
		{
			LOGINFO("Entering..!!!");
			if (nullptr != m_remoteXCastObj)
			{
				delete m_remoteXCastObj;
				m_remoteXCastObj = nullptr;
			}
		}

		// Thunder plugins communication
        void MiracastService::getXCastPlugin()
        {
			LOGINFO("Entering..!!!");

            if(nullptr == m_remoteXCastObj)
            {
				string token;

                // TODO: use interfaces and remove token
                auto security = m_CurrentService->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
                if (nullptr != security) {
                    string payload = "http://localhost";
                    if (security->CreateToken(
                            static_cast<uint16_t>(payload.length()),
                            reinterpret_cast<const uint8_t*>(payload.c_str()),
                            token)
                        == Core::ERROR_NONE) {
						LOGINFO("got security token\n");
                    }
					else{
						LOGERR("failed to get security token\n");
                    }
                    security->Release();
                } else {
					LOGERR("No security agent\n");
                }

                string query = "token=" + token;
                Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
                m_remoteXCastObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(XCAST_CALLSIGN_VER), (_T(XCAST_CALLSIGN_VER)), false, query);
				if (nullptr == m_remoteXCastObj) {
					LOGERR("JSONRPC: %s: initialization failed", XCAST_CALLSIGN_VER);
				}
				else{
					LOGINFO("JSONRPC: %s: initialization ok", XCAST_CALLSIGN_VER);
				}
            }
        }

		const string MiracastService::Initialize(PluginHost::IShell *service)
		{
			string msg;
			LOGINFO("Entering..!!!");
			if (!m_isServiceInitialized)
			{
				m_miracast_service_impl = MiracastServiceImplementation::create(this);

				if ( nullptr != m_miracast_service_impl ){
					std::string friendlyname = "";

					m_CurrentService = service;
					getXCastPlugin();		
					if (Core::ERROR_NONE == get_XCastFriendlyName(friendlyname))
					{
						m_miracast_service_impl->setFriendlyName(friendlyname);
					}
					m_isServiceInitialized = true;
				}
				else{
					msg = "Failed to obtain MiracastServiceImpl Object";
				}
			}

			// On success return empty, to indicate there is no error text.
			return msg;
		}

		void MiracastService::Deinitialize(PluginHost::IShell * /* service */)
		{
			MiracastService::_instance = nullptr;
			LOGINFO("Entering..!!!");

			if (m_isServiceInitialized)
			{
				m_miracast_service_impl->Shutdown();
				MiracastServiceImplementation::Destroy(m_miracast_service_impl);
				m_CurrentService = nullptr;
				m_miracast_service_impl = nullptr;
				m_isServiceInitialized = false;
				m_isServiceEnabled = false;
			}
		}

		string MiracastService::Information() const
		{
			return (string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
		}

		/**
		 * @brief This method used to Enable/Disable the Miracast/WFD Discovery.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::setEnable(const JsonObject &parameters, JsonObject &response)
		{
			bool success = false;
			bool is_enabled = true;

			LOGINFO("Entering..!!!");
			if (parameters.HasLabel("enabled"))
			{
				getBoolParameter("enabled", is_enabled);

				if ( true == is_enabled )
				{
					if (!m_isServiceEnabled)
					{
						m_miracast_service_impl->setEnable(is_enabled);
						success = true;
						m_isServiceEnabled = is_enabled;
						response["message"] = "Successfully enabled the WFD Discovery";
					}
					else
					{
						response["message"] = "WFD Discovery already enabled.";
					}
				}
				else
				{
					if (m_isServiceEnabled)
					{
						m_miracast_service_impl->setEnable(is_enabled);
						success = true;
						m_isServiceEnabled = is_enabled;
						response["message"] = "Successfully disabled the WFD Discovery";
					}
					else
					{
						response["message"] = "WFD Discovery already disabled.";
					}
				}
			}
			else
			{
				response["message"] = "Invalid parameter passed";
			}
			returnResponse(success);
		}

		/**
		 * @brief This method used to get the current state of Miracast/WFD Discovery.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::getEnable(const JsonObject &parameters, JsonObject &response)
		{
			LOGINFO("Entering..!!!");
			response["enabled"] = m_isServiceEnabled;
			returnResponse(true);
		}

		/**
		 * @brief This method used to accept or reject the WFD connection request.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::acceptClientConnection(const JsonObject &parameters, JsonObject &response)
		{
			bool success = false;
			std::string requestedStatus = "";

			LOGINFO("Entering..!!!");

			if (parameters.HasLabel("requestStatus"))
			{
				requestedStatus = parameters["requestStatus"].String();
				if (("Accept" == requestedStatus) || ("Reject" == requestedStatus))
				{
					m_miracast_service_impl->acceptClientConnectionRequest (requestedStatus);
					success = true;
				}
				else
				{
					response["message"] = "Supported 'requestStatus' parameter values are Accept or Reject";
					LOGERR("Unsupported param passed [%s].\n", requestedStatus.c_str());
				}
			}
			else
			{
				LOGERR("Invalid parameter passed");
				response["message"] = "Invalid parameter passed";
			}

			returnResponse(success);
		}

		/**
		 * @brief This method used to stop the client connection.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::stopClientConnection(const JsonObject &parameters, JsonObject &response)
		{
			bool success = false;
			std::string mac_addr = "";

			LOGINFO("Entering..!!!");

			if (parameters.HasLabel("clientMac"))
			{
				mac_addr = parameters["clientMac"].String();
				const std::regex mac_regex("^([0-9a-f]{2}[:]){5}([0-9a-f]{2})$");

				if (true == std::regex_match(mac_addr, mac_regex))
				{
					if (true == m_miracast_service_impl->StopClientConnection(mac_addr))
					{
						success = true;
						response["message"] = "Successfully Initiated the Stop WFD Client Connection";
						LOGINFO("Successfully Initiated the Stop WFD Client Connection");
					}
					else
					{
						LOGERR("MAC Address[%s] not connected yet",mac_addr.c_str());
						response["message"] = "MAC Address not connected yet.";
					}
				}
				else
				{
					LOGERR("Invalid MAC Address[%s] passed",mac_addr.c_str());
					response["message"] = "Invalid MAC Address";
				}
			}
			else
			{
				LOGERR("Invalid parameter passed");
				response["message"] = "Invalid parameter passed";
			}

			returnResponse(success);
		}

		void MiracastService::onMiracastServiceClientConnectionRequest(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_REQUEST, params);
		}

		void MiracastService::onMiracastServiceClientStopRequest(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_STOP_REQUEST, params);
		}

		void MiracastService::onMiracastServiceClientConnectionStarted(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_STARTED, params);
		}

		void MiracastService::onMiracastServiceClientConnectionError(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_ERROR, params);
		}

		int MiracastService::get_XCastFriendlyName(std::string &friendlyname)
		{
			JsonObject params, Result;
			LOGINFO("Entering..!!!");

			if (nullptr == m_remoteXCastObj)
			{
				LOGERR("m_remoteXCastObj not yet instantiated");
				return Core::ERROR_GENERAL;
			}

			uint32_t ret = m_remoteXCastObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getFriendlyName"), params, Result);

			if (Core::ERROR_NONE == ret)
			{
				if (Result["success"].Boolean())
				{
					friendlyname = Result["friendlyname"].String();
					LOGINFO("XCAST FriendlyName=%s", friendlyname.c_str());
				}
				else
				{
					ret = Core::ERROR_GENERAL;
					LOGERR("get_XCastFriendlyName call failed");
				}
			}
			else
			{
				LOGERR("get_XCastFriendlyName call failed E[%u]", ret);
			}
			return ret;
		}
	} // namespace Plugin
} // namespace WPEFramework