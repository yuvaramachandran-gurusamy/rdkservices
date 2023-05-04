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

#include "MiracastRtspMsg.h"
#include <MiracastLogger.h>

RTSP_MSG_TEMPLATE_INFO MiracastRTSPMsg::rtsp_msg_template_info[] = {
    {RTSP_MSG_FMT_M1_RESPONSE, "RTSP/1.0 200 OK\r\nPublic: \"%s, GET_PARAMETER, SET_PARAMETER\"\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M2_REQUEST, "OPTIONS * RTSP/1.0\r\nRequire: %s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M3_RESPONSE, "RTSP/1.0 200 OK\r\nContent-Length: %s\r\nContent-Type: text/parameters\r\nCSeq: %s\r\n\r\n%s"},
    {RTSP_MSG_FMT_M4_RESPONSE, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M5_RESPONSE, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M6_REQUEST, "SETUP %s RTSP/1.0\r\nTransport: %s;%sclient_port=%s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M7_REQUEST, "PLAY %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_M16_RESPONSE, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_PAUSE_REQUEST, "PAUSE %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_PLAY_REQUEST, "PLAY %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_TEARDOWN_REQUEST, "TEARDOWN %s RTSP/1.0\r\nSession: %s\r\nCSeq: %s\r\n\r\n"},
    {RTSP_MSG_FMT_TEARDOWN_RESPONSE, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n"}};

MiracastRTSPMsg::MiracastRTSPMsg()
{
    MIRACASTLOG_TRACE("Entering...");
    std::string default_configuration;

    m_current_sequence_number.clear();

    set_WFDEnableDisableUnicast(true);

    default_configuration = RTSP_DFLT_VIDEO_FORMATS;
    set_WFDVideoFormat(default_configuration);

    default_configuration = RTSP_DFLT_AUDIO_FORMATS;
    set_WFDAudioCodecs(default_configuration);

    default_configuration = RTSP_DFLT_CONTENT_PROTECTION;
    set_WFDContentProtection(default_configuration);

    default_configuration = RTSP_DFLT_TRANSPORT_PROFILE;
    set_WFDTransportProfile(default_configuration);

    default_configuration = RTSP_DFLT_STREAMING_PORT;
    set_WFDStreamingPortNumber(default_configuration);

    default_configuration = RTSP_DFLT_CLIENT_RTP_PORTS;
    set_WFDClientRTPPorts(default_configuration);
    MIRACASTLOG_TRACE("Exiting...");
}
MiracastRTSPMsg::~MiracastRTSPMsg()
{
}

std::string MiracastRTSPMsg::get_WFDVideoFormat(void)
{
    return m_wfd_video_formats;
}

std::string MiracastRTSPMsg::get_WFDAudioCodecs(void)
{
    return m_wfd_audio_codecs;
}

std::string MiracastRTSPMsg::get_WFDClientRTPPorts(void)
{
    return m_wfd_client_rtp_ports;
}

std::string MiracastRTSPMsg::get_WFDUIBCCapability(void)
{
    return m_wfd_uibc_capability;
}

std::string MiracastRTSPMsg::get_WFDContentProtection(void)
{
    return m_wfd_content_protection;
}

std::string MiracastRTSPMsg::get_WFDSecScreenSharing(void)
{
    return m_wfd_sec_screensharing;
}

std::string MiracastRTSPMsg::get_WFDPortraitDisplay(void)
{
    return m_wfd_sec_portrait_display;
}

std::string MiracastRTSPMsg::get_WFDSecRotation(void)
{
    return m_wfd_sec_rotation;
}

std::string MiracastRTSPMsg::get_WFDSecHWRotation(void)
{
    return m_wfd_sec_hw_rotation;
}

std::string MiracastRTSPMsg::get_WFDSecFrameRate(void)
{
    return m_wfd_sec_framerate;
}

std::string MiracastRTSPMsg::get_WFDPresentationURL(void)
{
    return m_wfd_presentation_URL;
}

std::string MiracastRTSPMsg::get_WFDTransportProfile(void)
{
    return m_wfd_transport_profile;
}

std::string MiracastRTSPMsg::get_WFDStreamingPortNumber(void)
{
    return m_wfd_streaming_port;
}

bool MiracastRTSPMsg::IsWFDUnicastSupported(void)
{
    return m_is_unicast;
}
std::string MiracastRTSPMsg::get_CurrentWFDSessionNumber(void)
{
    return m_wfd_session_number;
}

bool MiracastRTSPMsg::set_WFDVideoFormat(std::string video_formats)
{
    m_wfd_video_formats = video_formats;
    return true;
}

bool MiracastRTSPMsg::set_WFDAudioCodecs(std::string audio_codecs)
{
    m_wfd_audio_codecs = audio_codecs;
    return true;
}

bool MiracastRTSPMsg::set_WFDClientRTPPorts(std::string client_rtp_ports)
{
    m_wfd_client_rtp_ports = client_rtp_ports;
    return true;
}

bool MiracastRTSPMsg::set_WFDUIBCCapability(std::string uibc_caps)
{
    m_wfd_uibc_capability = uibc_caps;
    return true;
}

bool MiracastRTSPMsg::set_WFDContentProtection(std::string content_protection)
{
    m_wfd_content_protection = content_protection;
    return true;
}

bool MiracastRTSPMsg::set_WFDSecScreenSharing(std::string screen_sharing)
{
    m_wfd_sec_screensharing = screen_sharing;
    return true;
}

bool MiracastRTSPMsg::set_WFDPortraitDisplay(std::string portrait_display)
{
    m_wfd_sec_portrait_display = portrait_display;
    return true;
}

bool MiracastRTSPMsg::set_WFDSecRotation(std::string rotation)
{
    m_wfd_sec_rotation = rotation;
    return true;
}

bool MiracastRTSPMsg::set_WFDSecHWRotation(std::string hw_rotation)
{
    m_wfd_sec_hw_rotation = hw_rotation;
    return true;
}

bool MiracastRTSPMsg::set_WFDSecFrameRate(std::string framerate)
{
    m_wfd_sec_framerate = framerate;
    return true;
}

bool MiracastRTSPMsg::set_WFDPresentationURL(std::string URL)
{
    m_wfd_presentation_URL = URL;
    return true;
}

bool MiracastRTSPMsg::set_WFDTransportProfile(std::string profile)
{
    m_wfd_transport_profile = profile;
    return true;
}

bool MiracastRTSPMsg::set_WFDStreamingPortNumber(std::string port_number)
{
    m_wfd_streaming_port = port_number;
    return true;
}

bool MiracastRTSPMsg::set_WFDEnableDisableUnicast(bool enable_disable_unicast)
{
    m_is_unicast = enable_disable_unicast;
    return true;
}

bool MiracastRTSPMsg::set_WFDSessionNumber(std::string session)
{
    m_wfd_session_number = session;
    return true;
}

const char* MiracastRTSPMsg::get_RequestResponseFormat(RTSP_MSG_FMT_SINK2SRC format_type)
{
    int index = static_cast<RTSP_MSG_FMT_SINK2SRC>(format_type) - static_cast<RTSP_MSG_FMT_SINK2SRC>(RTSP_MSG_FMT_M1_RESPONSE);
    if (index >= 0 && index < static_cast<int>(sizeof(rtsp_msg_template_info) / sizeof(rtsp_msg_template_info[0])))
    {
        return rtsp_msg_template_info[index].template_name;
    }
    return "";
}

std::string MiracastRTSPMsg::generate_request_response_msg(RTSP_MSG_FMT_SINK2SRC msg_fmt_needed, std::string received_session_no, std::string append_data1)
{
    MIRACASTLOG_TRACE("Entering...");
    std::vector<const char *> sprintf_args;
    const char *template_str = get_RequestResponseFormat(msg_fmt_needed);
    std::string content_buffer = "";
    std::string unicast_supported = "";
    std::string content_buffer_len;
    std::string sequence_number = get_RequestSequenceNumber();
    std::string URL = get_WFDPresentationURL();
    std::string TSProfile = get_WFDTransportProfile();
    std::string StreamingPort = get_WFDStreamingPortNumber();
    std::string WFDSessionNum = get_CurrentWFDSessionNumber();

    // Determine the required buffer size using snprintf
    switch (msg_fmt_needed)
    {
    case RTSP_MSG_FMT_M1_RESPONSE:
    {
        sprintf_args.push_back(append_data1.c_str());
        sprintf_args.push_back(received_session_no.c_str());
    }
    break;
    case RTSP_MSG_FMT_M3_RESPONSE:
    {
        // prepare content buffer
        // Append content protection type
        content_buffer.append(RTSP_WFD_CONTENT_PROTECT_FIELD);
        content_buffer.append(get_WFDContentProtection());
        content_buffer.append(RTSP_CRLF_STR);
        // Append Video Formats
        content_buffer.append(RTSP_WFD_VIDEO_FMT_FIELD);
        content_buffer.append(get_WFDVideoFormat());
        content_buffer.append(RTSP_CRLF_STR);
        // Append Audio Formats
        content_buffer.append(RTSP_WFD_AUDIO_FMT_FIELD);
        content_buffer.append(get_WFDAudioCodecs());
        content_buffer.append(RTSP_CRLF_STR);
        // Append Client RTP Client port configuration
        content_buffer.append(RTSP_WFD_CLIENT_PORTS_FIELD);
        content_buffer.append(get_WFDClientRTPPorts());
        content_buffer.append(RTSP_CRLF_STR);

        content_buffer_len = std::to_string(content_buffer.length());

        sprintf_args.push_back(content_buffer_len.c_str());
        sprintf_args.push_back(received_session_no.c_str());
        sprintf_args.push_back(content_buffer.c_str());

        MIRACASTLOG_INFO("content_buffer - [%s]\n", content_buffer.c_str());
    }
    break;
    case RTSP_MSG_FMT_M4_RESPONSE:
    case RTSP_MSG_FMT_M5_RESPONSE:
    case RTSP_MSG_FMT_M16_RESPONSE:
    case RTSP_MSG_FMT_TEARDOWN_RESPONSE:
    {
        sprintf_args.push_back(received_session_no.c_str());
    }
    break;
    case RTSP_MSG_FMT_M2_REQUEST:
    case RTSP_MSG_FMT_M6_REQUEST:
    case RTSP_MSG_FMT_M7_REQUEST:
    case RTSP_MSG_FMT_PAUSE_REQUEST:
    case RTSP_MSG_FMT_PLAY_REQUEST:
    case RTSP_MSG_FMT_TEARDOWN_REQUEST:
    {
        if (RTSP_MSG_FMT_M2_REQUEST == msg_fmt_needed)
        {
            sprintf_args.push_back(append_data1.c_str());
        }
        else
        {
            sprintf_args.push_back(URL.c_str());

            if (RTSP_MSG_FMT_M6_REQUEST == msg_fmt_needed)
            {
                sprintf_args.push_back(TSProfile.c_str());
                if (true == IsWFDUnicastSupported())
                {
                    unicast_supported.append(RTSP_STD_UNICAST_FIELD);
                    unicast_supported.append(RTSP_SEMI_COLON_STR);
                    sprintf_args.push_back(unicast_supported.c_str());
                }
                sprintf_args.push_back(StreamingPort.c_str());
            }
            else
            {
                sprintf_args.push_back(WFDSessionNum.c_str());
            }
        }
        sprintf_args.push_back(sequence_number.c_str());
    }
    break;
    default:
    {
        MIRACASTLOG_ERROR("INVALID FMT REQUEST\n");
    }
    break;
    }

    std::string result = "";

    if (0 != sprintf_args.size())
    {
        result = MiracastRTSPMsg::format_string(template_str, sprintf_args);
    }
    MIRACASTLOG_TRACE("Exiting...");
    return result;
}

std::string MiracastRTSPMsg::get_RequestSequenceNumber(void)
{
    int next_number = std::stoi(m_current_sequence_number.empty() ? "0" : m_current_sequence_number) + 1;
    m_current_sequence_number = std::to_string(next_number);
    return m_current_sequence_number;
}

void MiracastRTSPMsg::set_WFDSourceMACAddress(std::string MAC_Addr)
{
    m_connected_mac_addr = MAC_Addr;
}

void MiracastRTSPMsg::set_WFDSourceName(std::string device_name)
{
    m_connected_device_name = device_name;
}

std::string MiracastRTSPMsg::get_WFDSourceName(void)
{
    return m_connected_device_name;
}

std::string MiracastRTSPMsg::get_WFDSourceMACAddress(void)
{
    return m_connected_mac_addr;
}

void MiracastRTSPMsg::reset_WFDSourceMACAddress(void)
{
    m_connected_mac_addr.clear();
}

void MiracastRTSPMsg::reset_WFDSourceName(void)
{
    m_connected_device_name.clear();
}