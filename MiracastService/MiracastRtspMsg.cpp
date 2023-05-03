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

MiracastRTSPMsg::MiracastRTSPMsg()
{
}
MiracastRTSPMsg::~MiracastRTSPMsg()
{
}

std::string MiracastRTSPMsg::get_WFDVideoFormat(void)
{
}

std::string MiracastRTSPMsg::get_WFDAudioCodecs(void)
{
}

std::string MiracastRTSPMsg::get_WFDClientRTPPorts(void)
{
}

std::string MiracastRTSPMsg::get_WFDUIBCCapability(void)
{
}

std::string MiracastRTSPMsg::get_WFDContentProtection(void)
{
}

std::string MiracastRTSPMsg::get_WFDSecScreenSharing(void)
{
}

std::string MiracastRTSPMsg::get_WFDPortraitDisplay(void)
{
}

std::string MiracastRTSPMsg::get_WFDSecRotation(void)
{
}

std::string MiracastRTSPMsg::get_WFDSecHWRotation(void)
{
}

std::string MiracastRTSPMsg::get_WFDSecFrameRate(void)
{
}

std::string MiracastRTSPMsg::get_WFDPresentationURL(void)
{
}

std::string MiracastRTSPMsg::get_WFDTransportProfile(void)
{
}

std::string MiracastRTSPMsg::get_WFDStreamingPortNumber(void)
{
}

bool MiracastRTSPMsg::IsWFDUnicastSupported(void)
{
}
std::string MiracastRTSPMsg::get_CurrentWFDSessionNumber(void)
{
}

bool MiracastRTSPMsg::set_WFDVideoFormat(std::string video_formats)
{
}

bool MiracastRTSPMsg::set_WFDAudioCodecs(std::string audio_codecs)
{
}

bool MiracastRTSPMsg::set_WFDClientRTPPorts(std::string client_rtp_ports)
{
}

bool MiracastRTSPMsg::set_WFDUIBCCapability(std::string uibc_caps)
{
}

bool MiracastRTSPMsg::set_WFDContentProtection(std::string content_protection)
{
}

bool MiracastRTSPMsg::set_WFDSecScreenSharing(std::string screen_sharing)
{
}

bool MiracastRTSPMsg::set_WFDPortraitDisplay(std::string portrait_display)
{
}

bool MiracastRTSPMsg::set_WFDSecRotation(std::string rotation)
{
}

bool MiracastRTSPMsg::set_WFDSecHWRotation(std::string hw_rotation)
{
}

bool MiracastRTSPMsg::set_WFDSecFrameRate(std::string framerate)
{
}

bool MiracastRTSPMsg::set_WFDPresentationURL(std::string URL)
{
}

bool MiracastRTSPMsg::set_WFDTransportProfile(std::string profile)
{
}

bool MiracastRTSPMsg::set_WFDStreamingPortNumber(std::string port_number)
{
}

bool MiracastRTSPMsg::set_WFDEnableDisableUnicast(bool enable_disable_unicast)
{
}

bool MiracastRTSPMsg::set_current_WFDSessionNumber(std::string session)
{
}

const char MiracastRTSPMsg::*get_RequestResponseFormat(RTSP_MSG_FMT_SINK2SRC format_type)
{
}

std::string MiracastRTSPMsg::generateRequestResponseFormat(RTSP_MSG_FMT_SINK2SRC msg_fmt_needed, std::string received_session_no, std::string append_data1)
{
}

std::string MiracastRTSPMsg::get_RequestSequenceNumber(void)
{
}

void MiracastRTSPMsg::set_WFDSourceMACAddress(std::string MAC_Addr)
{
}

void MiracastRTSPMsg::set_WFDSourceName(std::string device_name)
{
}

std::string MiracastRTSPMsg::get_WFDSourceName(void)
{
}

std::string MiracastRTSPMsg::get_WFDSourceMACAddress(void)
{
}

void MiracastRTSPMsg::reset_WFDSourceMACAddress(void)
{
}

void MiracastRTSPMsg::reset_WFDSourceName(void)
{
}
