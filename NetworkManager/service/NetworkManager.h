/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2022 RDK Management
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

#pragma once

#include "Module.h"

// Include the interface we created
#include <interfaces/INetworkManager.h>
//#include "INetworkManager.h"

#include <string>
#include <atomic>

namespace WPEFramework
{
    namespace Plugin
    {
        /**
         * NetworkManager plugin that exposes an API over both COM-RPC and JSON-RPC
         *
         */
        class NetworkManager : public PluginHost::IPlugin, public PluginHost::JSONRPC
#if 1
        , public PluginHost::ISubSystem::IInternet
#endif
        {
            /**
             * Our notification handling code
             *
             * Handle both the Activate/Deactivate notifications and provide a handler
             * for notifications raised by the COM-RPC API
             */
            class Notification : public RPC::IRemoteConnection::INotification,
                                 public Exchange::INetworkManager::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;
                string InterfaceStateToString(Exchange::INetworkManager::INotification::InterfaceState event)
                {
                    return "Interface_ADDED";
                }

            public:
                explicit Notification(NetworkManager *parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                virtual ~Notification() override
                {
                }

            public:
                void onInterfaceStateChanged(const Exchange::INetworkManager::INotification::InterfaceState event, const string interface) override
                {
                    JsonObject params;
                    params["interface"] = interface;
                    params["state"] = InterfaceStateToString(event);;
                    _parent.Notify("onInterfaceStateChanged", params);
                    printf ("%s\n", __FUNCTION__);
                }

                void onIPAddressChanged(const string interface, const bool isAcquired, const bool isIPv6, const string ipAddress) override
                {
                    JsonObject params;
                    params["interface"] = interface;
                    params["status"] = string (isAcquired ? "ACQUIRED" : "LOST");
                    if (isIPv6)
                        params["ipv6"] = ipAddress;
                    else
                        params["ipv4"] = ipAddress;

                    _parent.Notify("onIPAddressChanged", params);
                    printf ("%s\n", __FUNCTION__);
                }

                void onActiveInterfaceChanged(const string prevActiveInterface, const string currentActiveinterface) override
                {
                    printf ("%s\n", __FUNCTION__);
                }

                void onInternetStatusChanged(const Exchange::INetworkManager::InternetStatus oldState, const Exchange::INetworkManager::InternetStatus newstate) override
                {
                    printf ("%s\n", __FUNCTION__);
                }

                void onPingResponse(const string guid, const string pingStatistics) override
                {
                    printf ("%s\n", __FUNCTION__);
                    JsonObject params;
                    JsonObject result;

                    params.FromString(pingStatistics);
                    result["pingStatistics"] = params;
                    result["guid"] = guid;
                    _parent.Notify("onPingResponse", result);
                }

                void onTraceResponse(const string guid, const string traceResult) override
                {
                    JsonObject params;
                    JsonObject result;
                    printf ("%s\n", __FUNCTION__);

                    params.FromString(traceResult);
                    result["traceResult"] = params;
                    result["guid"] = guid;
                    _parent.Notify("onTraceResponse", result);
                }

                // WiFi Notifications that other processes can subscribe to
                void onAvailableSSIDs(const string jsonOfWiFiScanResults) override
                {
                    printf ("%s\n", __FUNCTION__);
                }
                void onWiFiStateChanged(const Exchange::INetworkManager::INotification::WiFiState state) override
                {
                    printf ("%s\n", __FUNCTION__);
                }
                void onWiFiSignalStrengthChanged(const string ssid, const string signalStrength, const Exchange::INetworkManager::WiFiSignalQuality quality) override
                {
                    printf ("%s\n", __FUNCTION__);
                }


                // The activated/deactived methods are part of the RPC::IRemoteConnection::INotification
                // interface. These are triggered when Thunder detects a connection/disconnection over the
                // COM-RPC link.
                void Activated(RPC::IRemoteConnection * /* connection */) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    // Something's caused the remote connection to be lost - this could be a crash
                    // on the remote side so deactivate ourselves
                    _parent.Deactivated(connection);
                }

                // Build QueryInterface implementation, specifying all possible interfaces we implement
                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::INetworkManager::INotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

            private:
                NetworkManager &_parent;
            };

        public:
            NetworkManager();
            ~NetworkManager() override;

            // Implement the basic IPlugin interface that all plugins must implement
            const string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            string Information() const override;

            // Do not allow copy/move constructors
            NetworkManager(const NetworkManager &) = delete;
            NetworkManager &operator=(const NetworkManager &) = delete;

            // Build QueryInterface implementation, specifying all possible interfaces we implement
            // This is necessary so that consumers can discover which plugin implements what interface
            BEGIN_INTERFACE_MAP(NetworkManager)

            // Which interfaces do we implement?
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
#if 1
            INTERFACE_ENTRY(PluginHost::ISubSystem::IInternet)
#endif

            // We need to tell Thunder that this plugin provides the INetworkManager interface, but
            // since it's not actually implemented here we tell Thunder where it can
            // find the real implementation
            // This allows other components to call QueryInterface<INetworkManager>() and
            // receive the actual implementation (which could be in-process or out-of-process)
            INTERFACE_AGGREGATE(Exchange::INetworkManager, _NetworkManager)
            END_INTERFACE_MAP

#if 1
            /*
            * ------------------------------------------------------------------------------------------------------------
            * ISubSystem::IInternet methods
            * ------------------------------------------------------------------------------------------------------------
            */
            string PublicIPAddress() const override
            {
                return m_publicIPAddress;
            }
            network_type NetworkType() const override
            {
                return (m_publicIPAddress.empty() == true ? PluginHost::ISubSystem::IInternet::UNKNOWN : (m_publicIPAddressType == "IPV6" ? PluginHost::ISubSystem::IInternet::IPV6 : PluginHost::ISubSystem::IInternet::IPV4));
            }
#endif
        private:
            // Notification/event handlers
            // Clean up when we're told to deactivate
            void Deactivated(RPC::IRemoteConnection *connection);

            // JSON-RPC setup
            void RegisterAllMethods();
            void UnregisterAllMethods();

            // JSON-RPC methods (take JSON in, spit JSON back out)
            uint32_t GetAvailableInterfaces (const JsonObject& parameters, JsonObject& response);
            uint32_t GetPrimaryInterface (const JsonObject& parameters, JsonObject& response);
            uint32_t SetPrimaryInterface (const JsonObject& parameters, JsonObject& response);
            uint32_t GetIPSettings(const JsonObject& parameters, JsonObject& response);
            uint32_t SetIPSettings(const JsonObject& parameters, JsonObject& response);
            uint32_t GetStunEndpoint(const JsonObject& parameters, JsonObject& response);
            uint32_t SetStunEndpoint(const JsonObject& parameters, JsonObject& response);
            uint32_t GetConnectivityTestEndpoints(const JsonObject& parameters, JsonObject& response);
            uint32_t SetConnectivityTestEndpoints(const JsonObject& parameters, JsonObject& response);
            uint32_t IsConnectedToInternet(const JsonObject& parameters, JsonObject& response);
            uint32_t GetCaptivePortalURI(const JsonObject& parameters, JsonObject& response);
            uint32_t StartConnectivityMonitoring(const JsonObject& parameters, JsonObject& response);
            uint32_t StopConnectivityMonitoring(const JsonObject& parameters, JsonObject& response);
            uint32_t GetPublicIP(const JsonObject& parameters, JsonObject& response);
            uint32_t Ping(const JsonObject& parameters, JsonObject& response);
            uint32_t Trace(const JsonObject& parameters, JsonObject& response);
            uint32_t StartWiFiScan(const JsonObject& parameters, JsonObject& response);
            uint32_t StopWiFiScan(const JsonObject& parameters, JsonObject& response);
            uint32_t GetKnownSSIDs(const JsonObject& parameters, JsonObject& response);
            uint32_t AddToKnownSSIDs(const JsonObject& parameters, JsonObject& response);
            uint32_t RemoveKnownSSID(const JsonObject& parameters, JsonObject& response);
            uint32_t WiFiConnect(const JsonObject& parameters, JsonObject& response);
            uint32_t WiFiDisconnect(const JsonObject& parameters, JsonObject& response);
            uint32_t GetConnectedSSID(const JsonObject& parameters, JsonObject& response);
            uint32_t StartWPS(const JsonObject& parameters, JsonObject& response);
            uint32_t StopWPS(const JsonObject& parameters, JsonObject& response);
            uint32_t GetWiFiSignalStrength(const JsonObject& parameters, JsonObject& response);
            uint32_t GetSupportedSecurityModes(const JsonObject& parameters, JsonObject& response);
 
        private:
            uint32_t _connectionId;
            PluginHost::IShell *_service;
            Exchange::INetworkManager *_NetworkManager;
            Core::Sink<Notification> _notification;
            string m_publicIPAddress;
            string m_publicIPAddressType;
        };
    }
}
