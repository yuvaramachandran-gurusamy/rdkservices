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

#define SERVER_DETAILS "127.0.0.1:9998"
#define SYSTEM_CALLSIGN "org.rdk.System"
#define SYSTEM_CALLSIGN_VER SYSTEM_CALLSIGN ".1"
#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 2000

/*Methods*/
const string WPEFramework::Plugin::MiracastPlayer::METHOD_MIRACAST_PLAYER_PLAY_REQUEST = "playRequest";
const string WPEFramework::Plugin::MiracastPlayer::METHOD_MIRACAST_PLAYER_STOP_REQUEST = "stopRequest";
const string WPEFramework::Plugin::MiracastPlayer::METHOD_MIRACAST_PLAYER_SET_VIDEO_RECTANGLE = "setVideoRectangle";

#define EVT_ON_STATE_CHANGE "onStateChange"

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
			MIRACAST::logger_init("MiracastPlayer");
			Register(METHOD_MIRACAST_PLAYER_PLAY_REQUEST, &MiracastPlayer::playRequest, this);
			Register(METHOD_MIRACAST_PLAYER_STOP_REQUEST, &MiracastPlayer::stopRequest, this);
			Register(METHOD_MIRACAST_PLAYER_SET_VIDEO_RECTANGLE, &MiracastPlayer::setVideoRectangle, this);
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
				m_miracast_rtsp_obj = MiracastRTSPMsg::getInstance(ret_code, this);
				if (nullptr != m_miracast_rtsp_obj)
				{
					m_CurrentService = service;
					m_GstPlayer = MiracastGstPlayer::getInstance();
					getSystemPlugin();
					m_isServiceInitialized = true;
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

		uint32_t MiracastPlayer::playRequest(const JsonObject &parameters, JsonObject &response)
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool success = false;
			LOGINFO("Entering..!!!");

			if(parameters.HasLabel("device_parameters")) {
				JsonObject device_parameters;
				std::string	source_dev_ip = "",
							source_dev_mac = "",
							source_dev_name = "",
							sink_dev_ip = "";

				device_parameters = parameters["device_parameters"].Object();

				source_dev_ip = device_parameters["source_dev_ip"].String();
				source_dev_mac = device_parameters["source_dev_mac"].String();
				source_dev_name = device_parameters["source_dev_name"].String();
				sink_dev_ip = device_parameters["sink_dev_ip"].String();

				strncpy( rtsp_hldr_msgq_data.source_dev_ip, source_dev_ip.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_ip));
				strncpy( rtsp_hldr_msgq_data.source_dev_mac, source_dev_mac.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_mac));
				strncpy( rtsp_hldr_msgq_data.source_dev_name, source_dev_name.c_str() , sizeof(rtsp_hldr_msgq_data.source_dev_name));
				strncpy( rtsp_hldr_msgq_data.sink_dev_ip, sink_dev_ip.c_str() , sizeof(rtsp_hldr_msgq_data.sink_dev_ip));

				rtsp_hldr_msgq_data.state = RTSP_START_RECEIVE_MSGS;
				success = true;
			}

			if(parameters.HasLabel("video_rectangle")) {
				JsonObject video_rectangle;
				unsigned int startX = 0,
							 startY = 0,
							 width = 0,
							 height = 0;

				video_rectangle = parameters["video_rectangle"].Object();

				startX = video_rectangle["X"].Number();
				startY = video_rectangle["Y"].Number();
				width = video_rectangle["W"].Number();
				height = video_rectangle["H"].Number();

				if (( 0 < width ) && ( 0 < height ))
				{
					m_video_sink_rect.startX = startX;
					m_video_sink_rect.startY = startY;
					m_video_sink_rect.width = width;
					m_video_sink_rect.height = height;

					rtsp_hldr_msgq_data.videorect = m_video_sink_rect;
					m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
				}
				else{
					success = false;
				}
			}

			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}

		uint32_t MiracastPlayer::stopRequest(const JsonObject &parameters, JsonObject &response)
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool success = true;
			LOGINFO("Entering..!!!");
			rtsp_hldr_msgq_data.state = RTSP_TEARDOWN_FROM_SINK2SRC;
			m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}

		uint32_t MiracastPlayer::setVideoRectangle(const JsonObject &parameters, JsonObject &response)
		{
			RTSP_HLDR_MSGQ_STRUCT rtsp_hldr_msgq_data = {0};
			bool success = false;
			LOGINFO("Entering..!!!");

			returnIfParamNotFound(parameters, "X");
			returnIfParamNotFound(parameters, "Y");
			returnIfParamNotFound(parameters, "W");
			returnIfParamNotFound(parameters, "H");

			unsigned int startX = 0,
						 startY = 0,
						 width = 0,
						 height = 0;

			startX = parameters["X"].Number();
			startY = parameters["Y"].Number();
			width = parameters["W"].Number();
			height = parameters["H"].Number();

			if (( 0 < width ) && ( 0 < height ) &&
				(( startX != m_video_sink_rect.startX ) ||
				( startY != m_video_sink_rect.startY ) ||
				( width != m_video_sink_rect.width ) ||
				( height != m_video_sink_rect.height )))
			{
				m_video_sink_rect.startX = startX;
				m_video_sink_rect.startY = startY;
				m_video_sink_rect.width = width;
				m_video_sink_rect.height = height;

				rtsp_hldr_msgq_data.videorect = m_video_sink_rect;
				rtsp_hldr_msgq_data.state = RTSP_UPDATE_VIDEO_RECT;
				m_miracast_rtsp_obj->send_msgto_rtsp_msg_hdler_thread(rtsp_hldr_msgq_data);
				success = true;
			}

			LOGINFO("Exiting..!!!");
			returnResponse(success);
		}

		void MiracastPlayer::onStateChange(string client_mac, string client_name, eMIRA_PLAYER_STATES player_state, eM_PLAYER_REASON_CODE reason_code)
		{
			LOGINFO("Entering..!!!");

			JsonObject params;
			params["mac"] = client_mac;
			params["name"] = client_name;
			params["state"] = stateDescription(player_state);
			params["reason_code"] = std::to_string(reason_code);
			params["reason"] = reasonDescription(reason_code);

			if (0 == access("/opt/miracast_autoconnect", F_OK)){
				std::string system_command = "";
				system_command = "curl -H \"Authorization: Bearer `WPEFrameworkSecurityUtility | cut -d '\"' -f 4`\"";
				system_command.append(" --header \"Content-Type: application/json\" --request POST --data '{\"jsonrpc\":\"2.0\", \"id\":3,\"method\":\"org.rdk.MiracastService.1.updatePlayerState\", \"params\":{");
				system_command.append("\"mac\": \"");
				system_command.append(client_mac);
				system_command.append("\",");
				system_command.append("\"state\": \"");
				system_command.append(stateDescription(player_state));
				system_command.append("\"}}}' http://127.0.0.1:9998/jsonrpc\n");

				MIRACASTLOG_INFO("System Command [%s]\n",system_command.c_str());
				system( system_command.c_str());
			}

			sendNotify(EVT_ON_STATE_CHANGE, params);
			LOGINFO("Exiting..!!!");
		}

		std::string MiracastPlayer::stateDescription(eMIRA_PLAYER_STATES e) throw()
		{
			switch (e)
			{
			case MIRACAST_PLAYER_STATE_IDLE:
				return "IDLE";
			case MIRACAST_PLAYER_STATE_INITIATED:
				return "INITIATED";
			case MIRACAST_PLAYER_STATE_INPROGRESS:
				return "INPROGRESS";
			case MIRACAST_PLAYER_STATE_PLAYING:
				return "PLAYING";
			case MIRACAST_PLAYER_STATE_STOPPED:
			case MIRACAST_PLAYER_STATE_SELF_ABORT:
				return "STOPPED";
			default:
				throw std::invalid_argument("Unimplemented item");
			}
		}

		std::string MiracastPlayer::reasonDescription(eM_PLAYER_REASON_CODE e) throw()
		{
			switch (e)
			{
			case MIRACAST_PLAYER_REASON_CODE_SUCCESS:
				return "SUCCESS";
			case MIRACAST_PLAYER_REASON_CODE_APP_REQ_TO_STOP:
				return "APP REQUESTED TO STOP.";
			case MIRACAST_PLAYER_REASON_CODE_RTSP_ERROR:
				return "RTSP Failure.";
			case MIRACAST_PLAYER_REASON_CODE_RTSP_TIMEOUT:
				return "RTSP Timeout.";
			case MIRACAST_PLAYER_REASON_CODE_GST_ERROR:
				return "GStreamer Failure.";
			case MIRACAST_PLAYER_REASON_CODE_INT_FAILURE:
				return "Internal Failure.";
			default:
				throw std::invalid_argument("Unimplemented item");
			}
		}

	} // namespace Plugin
} // namespace WPEFramework