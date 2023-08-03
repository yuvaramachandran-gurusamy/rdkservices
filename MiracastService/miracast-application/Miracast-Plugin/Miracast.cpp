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

#include "Miracast.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <cstdio>
#include "MiracastLogging.hpp"

#define SETTINGS_FILE_NAME "/opt/user_preferences.conf"
#define SETTINGS_FILE_KEY "ui_language"
#define SETTINGS_FILE_GROUP "General"

#define EXIT_APP_REQUESTED 79

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

using namespace std;

namespace WPEFramework
{
    namespace Miracast
    {
        class MemoryObserverImpl : public Exchange::IMemory
        {
        private:
            MemoryObserverImpl();
            MemoryObserverImpl(const MemoryObserverImpl &);
            MemoryObserverImpl &operator=(const MemoryObserverImpl &);

        public:
            MemoryObserverImpl(const RPC::IRemoteConnection *connection) : _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
            {
            }
            ~MemoryObserverImpl()
            {
            }

        public:
            virtual void Observe(const uint32_t pid)
            {
                /*Do nothing a dummy implementation to build with current thunder version*/
            }
            virtual uint64_t Resident() const
            {
                return _main.Resident();
            }
            virtual uint64_t Allocated() const
            {
                return _main.Allocated();
            }
            virtual uint64_t Shared() const
            {
                return _main.Shared();
            }
            virtual uint8_t Processes() const
            {
                return (IsOperational() ? 1 : 0);
            }
#ifdef USE_THUNDER_R4
            virtual bool IsOperational() const
            {
#else
            virtual const bool IsOperational() const
            {
#endif
                return _main.IsActive();
            }
            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP
        private:
            Core::ProcessInfo _main;
        };
        Exchange::IMemory *MemoryObserver(const RPC::IRemoteConnection *connection)
        {
            ASSERT(connection != nullptr);
            Exchange::IMemory *result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
            return (result);
        }
    } // namespace Miracast

    namespace Plugin
    {
        /* Instantiate the plugin. (exposes iplugin) */
        SERVICE_REGISTRATION(Miracast, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);
        const string Miracast::Initialize(PluginHost::IShell *service)
        {
            string result;
            TRACE(Trace::Information, (_T("Miracast plugin Initialize ")));
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);
            _service = service;
            _connectionId = 0;
            _service->Register(&_notification);
            _miracast = _service->Root<Exchange::IMiracast>(_connectionId, 2000, _T("MiracastImpl"));
            if (_miracast != nullptr)
            {
                PluginHost::IStateControl *stateControl(_miracast->QueryInterface<PluginHost::IStateControl>());
                if (stateControl == nullptr)
                {
                    Miracast_log(LOG_ERR, "%s: Miracast can't find IStateControl\n", MIRACAST_APP_LOG);
                    _miracast->Release();
                    _miracast = nullptr;
                    result = _T("Miracast can't find IStateControl instance");
                }
                else
                {
                    _memory = WPEFramework::Miracast::MemoryObserver(_service->RemoteConnection(_connectionId));
                    ASSERT(_memory != nullptr);
                    _miracast->Register(&_notification);
                    stateControl->Register(&_notification);
                    stateControl->Configure(_service);
                }
                stateControl->Release();
            }
            else
            {
                result = _T("Miracast Couldn't create _miracast instance");
                _service->Unregister(&_notification);
                _service = nullptr;
            }
            return (result);
        }
        void Miracast::Deinitialize(PluginHost::IShell *service)
        {
            Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Deinitialize() method invoked\n", MIRACAST_APP_LOG);
            printf("Miracast Plugin: Deinitialize() method invoked\n");
            fflush(stdout);
            TRACE(Trace::Information, (_T("Miracast Deinitialize ")));
            ASSERT(_service == service);
            ASSERT(_miracast != nullptr);
            ASSERT(_memory != nullptr);
            PluginHost::IStateControl *stateControl(_miracast->QueryInterface<PluginHost::IStateControl>());
            _service->Unregister(&_notification);
            _miracast->Unregister(&_notification);
            _memory->Release();
            if (stateControl != nullptr)
            {
                stateControl->Unregister(&_notification);
                stateControl->Release();
            }
            else
            {
                // On behalf of the crashed process, we will release the notification sink.
                _notification.Release();
            }
            if (_miracast->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED)
            {
                ASSERT(_connectionId != 0);
                TRACE(Trace::Error, (_T("Miracast Plugin is not destructed properly.")));
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin is not destructed properly.\n", MIRACAST_APP_LOG);
                printf("Miracast Plugin is not destructed properly.\n");
                fflush(stdout);
                RPC::IRemoteConnection *connection(_service->RemoteConnection(_connectionId));
                if (connection != nullptr)
                {
                    connection->Terminate();
                    connection->Release();
                }
            }
            _service = nullptr;
            _miracast = nullptr;
            _memory = nullptr;
        }
        string Miracast::Information() const
        {
            TRACE(Trace::Information, (_T("Miracast Plugin Information ")));
            return string();
        }
        void Miracast::Exit(const uint32_t exitCode)
        {
            TRACE(Trace::Information, (_T("Ariplay Plugin Exit code ")));
            Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Exit() method invoked\n", MIRACAST_APP_LOG);
            printf("Miracast Plugin: exitCode=%d\n", exitCode);
            fflush(stdout);
            if (exitCode == 0)
            {
                TRACE(Trace::Information, (_T("Ariplay Plugin Exit code 0 ")));
                printf("Ariplay Plugin Exit code 0 \n");
                fflush(stdout);
                ASSERT(_service != nullptr);
                PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::REQUESTED));
            }
            else if (exitCode == EXIT_APP_REQUESTED)
            {
                TRACE_L1("Miracast Plugin: exitCode=%d ", exitCode);
            }
            else
            {
                TRACE_L1("Miracast Plugin: exitCode=%d ", exitCode);
            }
        }
        void Miracast::Deactivated(RPC::IRemoteConnection *connection)
        {
            Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Deactivated() method invoked\n", MIRACAST_APP_LOG);
            if (connection->Id() == _connectionId)
            {
                ASSERT(_service != nullptr);
                PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
        void Miracast::Event(const string &eventName)
        {
            JsonObject params;
            params["EventName"] = eventName;
            event_notifyclient(eventName);
        }
        void Miracast::StateChange(const PluginHost::IStateControl::state state)
        {
            switch (state)
            {
            case PluginHost::IStateControl::RESUMED:
                TRACE(Trace::Information, (string(_T("StateChange: { \"suspend\":false }"))));
                _service->Notify("{ \"suspended\":false }");
                break;
            case PluginHost::IStateControl::SUSPENDED:
                TRACE(Trace::Information, (string(_T("StateChange: { \"suspend\":true }"))));
                _service->Notify("{ \"suspended\":true }");
                break;
            case PluginHost::IStateControl::EXITED:
            case PluginHost::IStateControl::UNINITIALIZED:
                break;
            default:
                ASSERT(false);
                break;
            }
            event_statechange(state == PluginHost::IStateControl::SUSPENDED ? true : false);
        }
    } // namespace Plugin
} // namespace WPEFramework