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
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_SET_VIDEO_FORMATS = "setVideoFormats";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_SET_AUDIO_FORMATS = "setAudioFormats";
const string WPEFramework::Plugin::MiracastService::METHOD_MIRACAST_SER_UPDATE_PLAYER_STATE = "updatePlayerState";

using namespace std;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

#define SERVER_DETAILS "127.0.0.1:9998"
#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN ".1"
#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 2000

#define EVT_ON_CLIENT_CONNECTION_REQUEST       "onClientConnectionRequest"
#define EVT_ON_CLIENT_CONNECTION_ERROR         "onClientConnectionError"
#define EVT_ON_LAUNCH_REQUEST                  "onLaunchRequest"

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
		MiracastController *MiracastService::m_miracast_ctrler_obj = nullptr;

		MiracastService::MiracastService()
			: PluginHost::JSONRPC(),
			  m_isServiceInitialized(false),
			  m_isServiceEnabled(false)
		{
			LOGINFO("Entering..!!!");
			MiracastService::_instance = this;
			MIRACAST::logger_init("MiracastService");

			Register(METHOD_MIRACAST_SET_ENABLE, &MiracastService::setEnable, this);
			Register(METHOD_MIRACAST_GET_ENABLE, &MiracastService::getEnable, this);
			Register(METHOD_MIRACAST_CLIENT_CONNECT, &MiracastService::acceptClientConnection, this);
#if 0
			Register(METHOD_MIRACAST_STOP_CLIENT_CONNECT, &MiracastService::stopClientConnection, this);
			Register(METHOD_MIRACAST_SET_VIDEO_FORMATS, &MiracastService::setVideoFormats, this);
			Register(METHOD_MIRACAST_SET_AUDIO_FORMATS, &MiracastService::setAudioFormats, this);
#endif
			Register(METHOD_MIRACAST_SER_UPDATE_PLAYER_STATE, &MiracastService::updatePlayerState, this);
			LOGINFO("Exiting..!!!");
		}

		MiracastService::~MiracastService()
		{
			LOGINFO("Entering..!!!");
			if (nullptr != m_SystemPluginObj)
			{
				delete m_SystemPluginObj;
				m_SystemPluginObj = nullptr;
			}
			Unregister(METHOD_MIRACAST_SET_ENABLE);
			Unregister(METHOD_MIRACAST_GET_ENABLE);
#if 0
			Unregister(METHOD_MIRACAST_STOP_CLIENT_CONNECT);
#endif
			Unregister(METHOD_MIRACAST_CLIENT_CONNECT);
			MIRACAST::logger_deinit();
			LOGINFO("Exiting..!!!");
		}

		// Thunder plugins communication
		void MiracastService::getSystemPlugin()
		{
			LOGINFO("Entering..!!!");

			if (nullptr == m_SystemPluginObj)
			{
				string token;
				// TODO: use interfaces and remove token
				auto security = m_CurrentService->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
				if (nullptr != security)
				{
					string payload = "http://localhost";
					if (security->CreateToken(static_cast<uint16_t>(payload.length()),
											  reinterpret_cast<const uint8_t *>(payload.c_str()),
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
				m_SystemPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(SYSTEM_CALLSIGN_VER), (_T("MiracastService")), false, query);
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

		const string MiracastService::Initialize(PluginHost::IShell *service)
		{
			string msg;
			LOGINFO("Entering..!!!");
			if (!m_isServiceInitialized)
			{
				MiracastError ret_code = MIRACAST_OK;
		
				m_miracast_ctrler_obj = MiracastController::getInstance(ret_code, this);
				if (nullptr != m_miracast_ctrler_obj)
				{
					m_CurrentService = service;
					getSystemPlugin();
					// subscribe for event
					m_SystemPluginObj->Subscribe<JsonObject>(1000, "onFriendlyNameChanged", &MiracastService::onFriendlyNameUpdateHandler, this);
					updateSystemFriendlyName();
					m_isServiceInitialized = true;
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_IDLE;
				}
				else
				{
					switch (ret_code)
					{
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
						msg = "Unknown Error:Failed to obtain MiracastController Object";
					}
					break;
					}
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
				MiracastController::destroyInstance();
				m_CurrentService = nullptr;
				m_miracast_ctrler_obj = nullptr;
				m_isServiceInitialized = false;
				m_isServiceEnabled = false;
				LOGINFO("Done..!!!");
			}
			LOGINFO("Exiting..!!!");
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

				if (true == is_enabled)
				{
					if (!m_isServiceEnabled)
					{
						m_miracast_ctrler_obj->set_enable(is_enabled);
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
						m_miracast_ctrler_obj->set_enable(is_enabled);
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
					m_miracast_ctrler_obj->accept_client_connection(requestedStatus);
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

#if 0
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

			if (parameters.HasLabel("mac"))
			{
				mac_addr = parameters["mac"].String();
				const std::regex mac_regex("^([0-9a-f]{2}[:]){5}([0-9a-f]{2})$");

				if (true == std::regex_match(mac_addr, mac_regex))
				{
					if (true == m_miracast_ctrler_obj->stop_client_connection(mac_addr))
					{
						success = true;
						response["message"] = "Successfully Initiated the Stop WFD Client Connection";
						LOGINFO("Successfully Initiated the Stop WFD Client Connection");
					}
					else
					{
						LOGERR("MAC Address[%s] not connected yet", mac_addr.c_str());
						response["message"] = "MAC Address not connected yet.";
					}
				}
				else
				{
					LOGERR("Invalid MAC Address[%s] passed", mac_addr.c_str());
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

		/**
		 * @brief This method used to set the videoformats for Miracast.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::setVideoFormats(const JsonObject &parameters, JsonObject &response)
		{
			JsonArray h264_codecs;
			RTSP_WFD_VIDEO_FMT_STRUCT st_video_fmt = {0};
			bool success = false;

			LOGINFO("Entering..!!!");

			returnIfParamNotFound(parameters, "native");
			returnIfBooleanParamNotFound(parameters, "display_mode_supported");
			returnIfParamNotFound(parameters, "h264_codecs");

			h264_codecs = parameters["h264_codecs"].Array();
			if (0 == h264_codecs.Length())
			{
				LOGWARN("Got empty list of h264_codecs");
				returnResponse(false);
			}
			getNumberParameter("native", st_video_fmt.native);
			getBoolParameter("display_mode_supported", st_video_fmt.preferred_display_mode_supported);

			JsonArray::Iterator index(h264_codecs.Elements());

			while (index.Next() == true)
			{
				if (Core::JSON::Variant::type::OBJECT == index.Current().Content())
				{
					JsonObject codecs = index.Current().Object();

					returnIfParamNotFound(codecs, "profile");
					returnIfParamNotFound(codecs, "level");
					returnIfParamNotFound(codecs, "cea_mask");
					returnIfParamNotFound(codecs, "vesa_mask");
					returnIfParamNotFound(codecs, "hh_mask");

					getNumberParameterObject(codecs, "profile", st_video_fmt.st_h264_codecs.profile);
					getNumberParameterObject(codecs, "level", st_video_fmt.st_h264_codecs.level);
					getNumberParameterObject(codecs, "cea_mask", st_video_fmt.st_h264_codecs.cea_mask);
					getNumberParameterObject(codecs, "vesa_mask", st_video_fmt.st_h264_codecs.vesa_mask);
					getNumberParameterObject(codecs, "hh_mask", st_video_fmt.st_h264_codecs.hh_mask);
					getNumberParameterObject(codecs, "latency", st_video_fmt.st_h264_codecs.latency);
					getNumberParameterObject(codecs, "min_slice", st_video_fmt.st_h264_codecs.min_slice);
					getNumberParameterObject(codecs, "slice_encode", st_video_fmt.st_h264_codecs.slice_encode);

					if (codecs.HasLabel("video_frame_skip_support"))
					{
						bool video_frame_skip_support;
						video_frame_skip_support = codecs["video_frame_skip_support"].Boolean();
						st_video_fmt.st_h264_codecs.video_frame_skip_support = video_frame_skip_support;
					}

					if (codecs.HasLabel("max_skip_intervals"))
					{
						uint8_t max_skip_intervals;
						getNumberParameterObject(codecs, "max_skip_intervals", max_skip_intervals);
						st_video_fmt.st_h264_codecs.max_skip_intervals = max_skip_intervals;
					}

					if (codecs.HasLabel("video_frame_rate_change_support"))
					{
						bool video_frame_rate_change_support;
						video_frame_rate_change_support = codecs["video_frame_rate_change_support"].Boolean();
						st_video_fmt.st_h264_codecs.video_frame_rate_change_support = video_frame_rate_change_support;
					}
				}
				else
					LOGWARN("Unexpected variant type");
			}
			success = m_miracast_ctrler_obj->set_WFDVideoFormat(st_video_fmt);

			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}

		/**
		 * @brief This method used to set the audioformats for Miracast.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::setAudioFormats(const JsonObject &parameters, JsonObject &response)
		{
			JsonArray audio_codecs;
			RTSP_WFD_AUDIO_FMT_STRUCT st_audio_fmt = {};
			bool success = false;

			LOGINFO("Entering..!!!");

			returnIfParamNotFound(parameters, "audio_codecs");

			audio_codecs = parameters["audio_codecs"].Array();
			if (0 == audio_codecs.Length())
			{
				LOGWARN("Got empty list of audio_codecs");
				returnResponse(false);
			}

			JsonArray::Iterator index(audio_codecs.Elements());

			while (index.Next() == true)
			{
				if (Core::JSON::Variant::type::OBJECT == index.Current().Content())
				{
					JsonObject codecs = index.Current().Object();

					returnIfParamNotFound(codecs, "audio_format");
					returnIfParamNotFound(codecs, "modes");
					returnIfParamNotFound(codecs, "latency");

					getNumberParameterObject(codecs, "audio_format", st_audio_fmt.audio_format);
					getNumberParameterObject(codecs, "modes", st_audio_fmt.modes);
					getNumberParameterObject(codecs, "latency", st_audio_fmt.latency);
				}
				else
					LOGWARN("Unexpected variant type");
			}
			success = m_miracast_ctrler_obj->set_WFDAudioCodecs(st_audio_fmt);

			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}
#endif
		/**
		 * @brief This method used to update the Player State for MiracastService Plugin.
		 *
		 * @param: None.
		 * @return Returns the success code of underlying method.
		 */
		uint32_t MiracastService::updatePlayerState(const JsonObject &parameters, JsonObject &response)
		{
			string mac;
			string player_state;
			RTSP_WFD_AUDIO_FMT_STRUCT st_audio_fmt = {};
			bool success = true;

			LOGINFO("Entering..!!!");

			returnIfStringParamNotFound(parameters, "mac");
			returnIfStringParamNotFound(parameters, "state");
			if (parameters.HasLabel("mac"))
			{
				getStringParameter("mac", mac);
			}
			if (parameters.HasLabel("state"))
			{
				getStringParameter("state", player_state);
				if (player_state == "PLAYING" || player_state == "playing")
				{
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_PLAYING;
				}
				else if (player_state == "STOPPED" || player_state == "stopped")
				{
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_STOPPED;
				}
				else if (player_state == "INITIATED" || player_state == "initiated")
				{
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_INITIATED;
				}
				else if (player_state == "INPROGRESS" || player_state == "inprogress")
				{
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_INPROGRESS;
				}
				else
				{
					m_miracast_ctrler_obj->m_ePlayer_state = MIRACAST_PLAYER_STATE_IDLE;
				}
			}

			if ( m_isServiceEnabled && ( MIRACAST_PLAYER_STATE_STOPPED == m_miracast_ctrler_obj->m_ePlayer_state ))
			{
				// It will restart the discovering
				m_miracast_ctrler_obj->restart_session_discovery();
			}

			LOGINFO("Player State set to [%s (%d)] for Source device [%s].", player_state.c_str(), (int)m_miracast_ctrler_obj->m_ePlayer_state, mac.c_str());
			// @TODO: Need to check what to do next?

			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}

		void MiracastService::onMiracastServiceClientConnectionRequest(string client_mac, string client_name)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["mac"] = client_mac;
			params["name"] = client_name;
			sendNotify(EVT_ON_CLIENT_CONNECTION_REQUEST, params);
		}

		void MiracastService::onMiracastServiceClientConnectionError(string client_mac, string client_name , eMIRACAST_SERVICE_ERR_CODE error_code )
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["mac"] = client_mac;
			params["name"] = client_name;
			params["error_code"] = std::to_string(error_code);
			params["reason"] = reasonDescription(error_code);
			sendNotify(EVT_ON_CLIENT_CONNECTION_ERROR, params);
		}

		std::string MiracastService::reasonDescription(eMIRACAST_SERVICE_ERR_CODE e) throw()
		{
			switch (e)
			{
			case MIRACAST_SERVICE_ERR_CODE_SUCCESS:
				return "SUCCESS";
			case MIRACAST_SERVICE_ERR_CODE_P2P_GROUP_NEGO_ERROR:
				return "P2P GROUP NEGOTIATION FAILURE.";
			case MIRACAST_SERVICE_ERR_CODE_P2P_GROUP_FORMATION_ERROR:
				return "P2P GROUP FORMATION FAILURE.";
			case MIRACAST_SERVICE_ERR_CODE_GENERIC_FAILURE:
				return "P2P GENERIC FAILURE.";
			case MIRACAST_SERVICE_ERR_CODE_P2P_CONNECT_ERROR:
				return "P2P CONNECT FAILURE.";
			default:
				throw std::invalid_argument("Unimplemented item");
			}
		}

		int MiracastService::updateSystemFriendlyName()
		{
			JsonObject params, Result;
			LOGINFO("Entering..!!!");

			if (nullptr == m_SystemPluginObj)
			{
				LOGERR("m_SystemPluginObj not yet instantiated");
				return Core::ERROR_GENERAL;
			}

			uint32_t ret = m_SystemPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getFriendlyName"), params, Result);

			if (Core::ERROR_NONE == ret)
			{
				if (Result["success"].Boolean())
				{
					std::string friendlyName = "";
					friendlyName = Result["friendlyName"].String();
					m_miracast_ctrler_obj->set_FriendlyName(friendlyName);
					LOGINFO("Miracast FriendlyName=%s", friendlyName.c_str());
				}
				else
				{
					ret = Core::ERROR_GENERAL;
					LOGERR("updateSystemFriendlyName call failed");
				}
			}
			else
			{
				LOGERR("updateSystemFriendlyName call failed E[%u]", ret);
			}
			return ret;
		}

		void MiracastService::onFriendlyNameUpdateHandler(const JsonObject &parameters)
		{
			string message;
			string value;
			parameters.ToString(message);
			LOGINFO("[Friendly Name Event], %s : %s", __FUNCTION__, message.c_str());

			if (parameters.HasLabel("friendlyName"))
			{
				value = parameters["friendlyName"].String();
				m_miracast_ctrler_obj->set_FriendlyName(value, m_isServiceEnabled);
				LOGINFO("Miracast FriendlyName=%s", value.c_str());
			}
		}

		void MiracastService::onMiracastServiceLaunchRequest(string src_dev_ip, string src_dev_mac, string src_dev_name, string sink_dev_ip)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			JsonObject device_params;
			device_params["source_dev_ip"] = src_dev_ip;
			device_params["source_dev_mac"] = src_dev_mac;
			device_params["source_dev_name"] = src_dev_name;
			device_params["sink_dev_ip"] = sink_dev_ip;
			params["device_parameters"] = device_params;

			if (0 == access("/opt/miracast_autoconnect", F_OK)){
				std::string system_command = "";
				system_command = "curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\"";
				
				system_command.append(" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastPlayer.1.playRequest\", \"params\":{");
				
				system_command.append("\"device_parameters\": {\n");

				system_command.append("\"source_dev_ip\": \"");
				system_command.append(src_dev_ip);
				system_command.append("\",\n");

				system_command.append("\"source_dev_mac\": \"");
				system_command.append(src_dev_mac);
				system_command.append("\",\n");

				system_command.append("\"source_dev_name\": \"");
				system_command.append(src_dev_name);
				system_command.append("\",\n");

				system_command.append("\"sink_dev_ip\": \"");
				system_command.append(sink_dev_ip);
				system_command.append("\"\n},\n");

				system_command.append("\"video_rectangle\": {\n");

				system_command.append("\"X\": ");
				system_command.append("0");
				system_command.append(",\n");

				system_command.append("\"Y\": ");
				system_command.append("0");
				system_command.append(",\n");

				system_command.append("\"W\": ");
				system_command.append("1920");
				system_command.append(",\n");

				system_command.append("\"H\": ");
				system_command.append("1080");

				system_command.append("}}}' http://127.0.0.1:9998/jsonrpc\n");

				MIRACASTLOG_INFO("System Command [%s]\n",system_command.c_str());
				system( system_command.c_str());
			}

			sendNotify(EVT_ON_LAUNCH_REQUEST, params);
		}
	} // namespace Plugin
} // namespace WPEFramework