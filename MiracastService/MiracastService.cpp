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
#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"
#include "MiracastService.h"

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

#define XCAST_CALLSIGN "org.rdk.Xcast.1"
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
			LOGINFO("MiracastService::ctor");
			MiracastService::_instance = this;
			/* Create Thunder Security token */
			std::string sToken;
			std::string query;

			/* If we're in a container, get the token from the Dobby env var */

			/* @TODO: Compare with other thunder plugin, like DisplaySetting */
			Core::SystemInfo::GetEnvironment(_T("THUNDER_SECURITY_TOKEN"), sToken);
			if (sToken.empty())
			{
				unsigned char buffer[SECURITY_TOKEN_LEN_MAX] = {0};
				int ret = GetSecurityToken(SECURITY_TOKEN_LEN_MAX, buffer);

				if (ret > 0)
				{
					sToken = (char *)buffer;
				}
			}
			query = "token=" + sToken;

			LOGINFO("Instantiating remote object of rdkservices plugins\n");

			remoteObjectXCast = new WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>(XCAST_CALLSIGN, "", false, query);
			Register(METHOD_MIRACAST_SET_ENABLE, &MiracastService::setEnable, this);
			Register(METHOD_MIRACAST_GET_ENABLE, &MiracastService::getEnable, this);
			Register(METHOD_MIRACAST_STOP_CLIENT_CONNECT, &MiracastService::stopClientConnection, this);
			Register(METHOD_MIRACAST_CLIENT_CONNECT, &MiracastService::acceptClientConnection, this);
		}

		MiracastService::~MiracastService()
		{
			LOGINFO("MiracastService::~MiracastService");
			if (nullptr != remoteObjectXCast)
			{
				delete remoteObjectXCast;
				remoteObjectXCast = nullptr;
			}
		}

		const string MiracastService::Initialize(PluginHost::IShell *service)
		{
			LOGINFO("MiracastService::Initialize");
			if (!m_isServiceInitialized)
			{
				std::string friendlyname = "";

				m_CurrentService = service;
				m_miracast_service_impl = MiracastServiceImplementation::create(this);
				if (Core::ERROR_NONE == get_XCastFriendlyName(friendlyname))
				{
					m_miracast_service_impl->setFriendlyName(friendlyname);
				}
				m_isServiceInitialized = true;
			}

			// On success return empty, to indicate there is no error text.
			/* @TODO: Check return type on success*/
			return (string());
		}

		void MiracastService::Deinitialize(PluginHost::IShell * /* service */)
		{
			MiracastService::_instance = nullptr;
			LOGINFO("MiracastService::Deinitialize");

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
			std::string is_enabled = "";
            //bool is_enabled = true;

			LOGINFO("MiracastService::setEnable");
			if (parameters.HasLabel("enabled"))
			{
				/*@TODO : Fix to bool*/
				/* getBoolParameter("true") */
				/* @todo: Fix log*/
				//is_enabled = getBoolParameter("true", is_enabled);
				if ("true" == is_enabled)
				{
					if (!m_isServiceEnabled)
					{
						m_miracast_service_impl->setEnable(is_enabled);
						success = true;
						m_isServiceEnabled = true;
						response["message"] = "Successfully enabled the WFD Discovery";
					}
					else
					{
						response["message"] = "WFD Discovery already enabled.";
					}
				}
				else if ("false" == is_enabled)
				{
					if (m_isServiceEnabled)
					{
						m_miracast_service_impl->setEnable(is_enabled);
						success = true;
						m_isServiceEnabled = false;
						response["message"] = "Successfully disabled the WFD Discovery";
					}
					else
					{
						response["message"] = "WFD Discovery already disabled.";
					}
				}
				else
				{
					response["message"] = "Supported 'enabled' parameter values are true or false";
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
			LOGINFO("MiracastService::getEnable");
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

			LOGINFO("MiracastService::acceptClientConnection");

			if (parameters.HasLabel("requestStatus"))
			{
				requestedStatus = parameters["requestStatus"].String();
				if (("Accept" == requestedStatus) || ("Reject" == requestedStatus))
				{
					m_miracast_service_impl->acceptClientConnection(requestedStatus);
					success = true;
				}
				else
				{
					response["message"] = "Supported 'requestStatus' parameter values are Accept or Reject";
					LOGERR("Unsupported param passed [%s].\n", requestedStatus);
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
		/*@TODO: Add Log messages*/
		uint32_t MiracastService::stopClientConnection(const JsonObject &parameters, JsonObject &response)
		{
			bool success = false;
			std::string mac_addr = "";

			LOGINFO("MiracastService::stopClientConnection");

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
					}
					else
					{
						response["message"] = "MAC Address not connected yet.";
					}
				}
				else
				{
					response["message"] = "Invalid MAC Address";
				}
			}
			else
			{
				response["message"] = "Invalid parameter passed";
			}

			returnResponse(success);
		}

		void MiracastService::onMiracastServiceClientConnectionRequest(string client_mac, string client_name)
		{
			LOGINFO("MiracastService::onMiracastServiceClientConnectionRequest ");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_REQUEST, params);
		}

		void MiracastService::onMiracastServiceClientStopRequest(string client_mac, string client_name)
		{
			LOGINFO("MiracastService::onMiracastServiceClientStopRequest ");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_STOP_REQUEST, params);
		}

		void MiracastService::onMiracastServiceClientConnectionStarted(string client_mac, string client_name)
		{
			LOGINFO("MiracastService::onMiracastServiceClientConnectionStarted ");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_STARTED, params);
		}

		void MiracastService::onMiracastServiceClientConnectionError(string client_mac, string client_name)
		{
			LOGINFO("MiracastService::onMiracastServiceClientConnectionError ");

			JsonObject params;
			params["clientMac"] = client_mac;
			params["clientName"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_ERROR, params);
		}

		int MiracastService::get_XCastFriendlyName(std::string &friendlyname)
		{
			JsonObject params, Result;
			LOGINFO("Entering..!!!");

			if (nullptr == remoteObjectXCast)
			{
				LOGINFO("remoteObjectXCast not yet instantiated");
				return Core::ERROR_GENERAL;
			}

			uint32_t ret = remoteObjectXCast->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getFriendlyName"), params, Result);

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
					LOGINFO("get_XCastFriendlyName call failed");
				}
			}
			else
			{
				LOGINFO("get_XCastFriendlyName call failed E[%u]", ret);
			}
			return ret;
		}
	} // namespace Plugin
} // namespace WPEFramework
