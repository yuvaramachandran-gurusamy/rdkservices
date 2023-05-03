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

#include "MiracastController.h"
#include "MiracastPlayer.h"

/* p2p communication*/
/* RTSP : M1-M16*/
/* Steaming Play*/

using namespace MIRACAST;

MiracastController::MiracastController()
{
}

MiracastController::~MiracastController()
{
}

MiracastError MiracastController::discover_devices()
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

MiracastError MiracastController::connect_device(std::string mac)
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

MiracastError MiracastController::start_streaming()
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
    return MIRACAST_OK;
}

std::string MiracastController::get_localIp()
{
    return m_localIp;
}

std::string MiracastController::get_wfd_streaming_port_number()
{
}

std::string MiracastController::get_connected_device_mac()
{
}

std::vector<DeviceInfo *> MiracastController::get_allPeers()
{
}

bool MiracastController::get_connection_status()
{
}

DeviceInfo MiracastController::*get_device_details(std::string mac)
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

MiracastError MiracastController::stop_streaming()
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

MiracastError MiracastController::disconnect_device()
{
}

MiracastError MiracastController::stop_discover_devices()
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

void MiracastController::restart_session(void)
{
}

void MiracastController::stop_session(void)
{
}

std::string MiracastController::get_device_name(std::string mac)
{
}

void MiracastController::event_handler(enum P2P_EVENTS eventId, void *data, size_t len)
{
}

/*Rename*/
void MiracastController::SendMessageToClientReqHandlerThread(size_t action, std::string action_buffer, std::string user_data)
{
}

// Session Manager
void MiracastController::SessionManager_Thread(void *args)
{
}

// RTSP Message Handler
void MiracastController::RTSPMessageHandler_Thread(void *args)
{
}

// Client Request Handler
void MiracastController::ClientRequestHandler_Thread(void *args)
{
}

RTSP_CTRL_SOCKET_STATES MiracastController::receive_buffer_timedOut(int sockfd, void *buffer, size_t buffer_len)
{
}

bool MiracastController::send_buffer_timedOut(int sockfd, std::string rtsp_response_buffer)
{
}

MiracastError MiracastController::executeCommand(std::string command, int interface, std::string &retBuffer)
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

std::string MiracastController::store_data(const char *tmpBuff, const char *lookup_data)
{
}

std::string MiracastController::start_DHCPClient(std::string interface, std::string &default_gw_ip_addr)
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
}

MiracastError MiracastController::initiate_TCP(std::string go_ip)
{
    MIRACASTLOG_TRACE("Entering..");

    MIRACASTLOG_TRACE("Exiting..");
    return MIRACAST_OK;
}

MiracastError MiracastController::connect_Sink()
{
    return MIRACAST_OK;
}

void MiracastController::wfdInit(MiracastServiceNotifier *notifier)
{
}

void MiracastController::set_localIp(std::string ipAddr)
{
    m_localIp = ipAddr;
}

MiracastError MiracastController::wait_data_timeout(int m_Sockfd, unsigned ms)
{
    return MIRACAST_OK;
}

RTSP_STATUS MiracastController::validate_rtsp_msg_response_back(std::string rtsp_msg_buffer, RTSP_MSG_HANDLER_ACTIONS action_id)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m1_msg_m2_send_request(std::string rtsp_m1_msg_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}
RTSP_STATUS MiracastController::validate_rtsp_m2_request_ack(std::string rtsp_m1_response_ack_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m3_response_back(std::string rtsp_m3_msg_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m4_response_back(std::string rtsp_m4_msg_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m5_msg_m6_send_request(std::string rtsp_m5_msg_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m6_ack_m7_send_request(std::string rtsp_m6_ack_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_m7_request_ack(std::string rtsp_m7_ack_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::validate_rtsp_post_m1_m7_xchange(std::string rtsp_post_m1_m7_xchange_buffer)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

RTSP_STATUS MiracastController::rtsp_sink2src_request_msg_handling(RTSP_MSG_HANDLER_ACTIONS action_id)
{
    RTSP_STATUS status = RTSP_MSG_SUCCESS;

    return status;
}

SESSION_MANAGER_ACTIONS MiracastController::convertP2PtoSessionActions(enum P2P_EVENTS eventId)
{
}
