#pragma once
#include "Module.h"
#include "string.h"
#include <interfaces/IMiracast.h>
#include <interfaces/IMemory.h>
#include <interfaces/json/JsonData_MiracastJSONRPC.h>
#include <interfaces/json/JsonData_StateControl.h>

namespace WPEFramework
{
    namespace Plugin
    {
        using namespace JsonData::MiracastJSONRPC;
        class Miracast : public PluginHost::IPlugin, public PluginHost::JSONRPC
        {
        public:
            Miracast(const Miracast &) = delete;
            Miracast &operator=(const Miracast &) = delete;

        private:
            class Notification
                : public RPC::IRemoteConnection::INotification,
                  public PluginHost::IStateControl::INotification,
                  public Exchange::IMiracast::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;

            public:
                explicit Notification(Miracast *parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~Notification()
                {
                }

            public:
                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
                INTERFACE_ENTRY(Exchange::IMiracast::INotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

            private:
                virtual void StateChange(const PluginHost::IStateControl::state state)
                {
                    _parent.StateChange(state);
                }
                virtual void StateChange(const Exchange::IMiracast::state state)
                {
                    _parent.StateChange(state);
                }
                virtual void Event(const string &eventName) override
                {
                    _parent.Event(eventName);
                }
                virtual void Activated(RPC::IRemoteConnection *)
                {
                }
                virtual void Deactivated(RPC::IRemoteConnection *connection)
                {
                    _parent.Deactivated(connection);
                }
                virtual void Exit(const uint32_t exitCode)
                {
                    _parent.Exit(exitCode);
                }

            private:
                Miracast &_parent;
            };

        public:
            //   IPlugin method stubs all are required!!!! (But not used)
            const string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            virtual string Information() const override;
            uint32_t _StartApp(const JsonData::MiracastJSONRPC::StartappParamsInfo &params);
            uint32_t _StopApp(void);
            Miracast() : _notification(this)
            {
                TRACE(Trace::Information, (_T("Miracast Register the API Name " )));
                RegisterAll();
            }
            virtual ~Miracast() override
            {
                TRACE(Trace::Information, (_T("Miracast Unregister the API Name " )));
                UnregisterAll();
                _Miracast = nullptr;
                _memory = nullptr;
            }

        public:
            // comRPC Interface, all are required.
            BEGIN_INTERFACE_MAP(Miracast)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_AGGREGATE(PluginHost::IStateControl, _Miracast)
            INTERFACE_AGGREGATE(Exchange::IMiracast, _miracast)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            // INTERFACE_ENTRY(Exchange::IMiracast::INotification)
            END_INTERFACE_MAP
        private:
            uint32_t _connectionId;
            Exchange::IMiracast *_miracast = nullptr;
            Exchange::IMemory *_memory = nullptr;
            PluginHost::IShell *_service = nullptr;
            Core::Sink<Notification> _notification;

            // Registration
            void RegisterAll();
            void UnregisterAll();

            // Activation
            void Deactivated(RPC::IRemoteConnection *connection);

            // Destruction
            void Exit(const uint32_t exitCode);

            // state control
            uint32_t get_state(Core::JSON::EnumType<JsonData::StateControl::StateType> &response) const; // StateControl
            uint32_t set_state(const Core::JSON::EnumType<JsonData::StateControl::StateType> &param);    // StateControl
            void StateChange(const PluginHost::IStateControl::state state);
            void StateChange(const Exchange::IMiracast::state state);

            // visibility
            uint32_t set_visibility(const Core::JSON::EnumType<JsonData::MiracastJSONRPC::VisibilityType> &param);

            // Notify client
            void Event(const string &eventName);

            // events
            void event_statechange(const bool &suspended);
            void event_visibilitychange(const bool &hidden);
            void event_playbackchange(const bool &playing);
            void event_notifyclient(const string &eventName);
        };
    } // namespace Plugin
} // namespace WPEFramework