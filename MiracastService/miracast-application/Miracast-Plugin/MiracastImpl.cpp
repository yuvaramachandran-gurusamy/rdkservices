#pragma once
#include <memory>

#include "Module.h"
#include <interfaces/IMiracast.h>
#include "AirPlayAppInterface.hpp"
#include "AirPlayLogging.hpp"

const int kExceptionSignals[] = {
    SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTERM, SIGINT, SIGALRM};

const int kNumHandledSignals =
    sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals];

void SignalHandler_callback(int sig);

namespace WPEFramework
{
    namespace Plugin
    {

        class MiracastImpl;
        static MiracastImpl *implementation = nullptr;
        class MiracastImpl : public Exchange::IMiracast, public Core::Thread, public PluginHost::IStateControl /*, public Exchange::IFocus*/
        {
        public:
            pthread_t priorityThreadId = -1;
            unsigned int priorityTimeout = 0;
            pthread_t exitThreadId;
            int exitReason;
            enum class ExternalEvent
            {
                SUSPEND,
                RESUME
            };

            MiracastImpl() : _state(PluginHost::IStateControl::UNINITIALIZED), _startSuspended(true), _adminLock(), _stateControlClients(), _miracastClients(), _isAppForceStopped(false), _options("MiracastApp")
            {
                // Miracast can only be instantiated once (it is a process wide singleton !!!!)
                ASSERT(implementation == nullptr);
                implementation = this;
            }
            virtual ~MiracastImpl()
            {
                Miracast_log(LOG_INFO, "%s: Block MiracastImpl destructor call", AIRPLAY_APP_LOG);
                if (_isAppForceStopped != true)
                {
                    const std::string str = "stopapp";
                    Miracast_log(LOG_INFO, "%s: MiracastImpl destructor: Invoking App Stop() method\n", AIRPLAY_APP_LOG);
                    this->StopApp(str);
                }
                Block();
                Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
                implementation = nullptr;
            }

            virtual uint32_t SystemCommand(const string &command)
            {
                return Core::ERROR_NONE;
            }

            virtual void SetVisible(bool visibility)
            {
                Miracast_log(LOG_INFO, "AirPlay::%s Invoked by RDKShell via SetVisibility\n", __func__);
                appInterface_setAppVisibility(visibility);
            }

            void Event(const string &eventName)
            {
                // ToDo: notify each miracast client about this event
                _adminLock.Lock();
                _adminLock.Unlock();
            }

            void SetSuspended(bool suspend)
            {
                if (suspend == true)
                {
                    appInterface_setAppVisibility(false);
                    Miracast_log(LOG_INFO, "%s: Miracast plugin state is going to set as Suspend\n", AIRPLAY_APP_LOG);
                    StateChange(PluginHost::IStateControl::SUSPENDED);
                }
                else
                {
                    appInterface_setAppVisibility(true);
                    Miracast_log(LOG_INFO, "%s: Miracast plugin state is going to set as Resumed\n", AIRPLAY_APP_LOG);
                    StateChange(PluginHost::IStateControl::RESUMED);
                }
            }

            virtual void FactoryReset()
            {
            }

            virtual uint32_t StartApp(const string &paramStr)
            {
                string argc_str;
                uint32_t err = Core::ERROR_NONE;
                JsonObject parameters;
                parameters.FromString(paramStr);
                TRACE(Trace::Information, (_T("startMiracastApp: ")));
                // SYSLOG(Trace::Information,(_T("Miracast Plugin: start AirPlay app via Miracast Plugin")));
                Miracast_log(LOG_INFO, "%s:Miracast Plugin: start AirPlay app via Miracast Plugin", AIRPLAY_APP_LOG);
                if (parameters.HasLabel("argc"))
                {
                    argc_str = parameters["argc"].String();
                    mArgc = stoi(argc_str);
                    mArgv = new const char *[mArgc];
                    string key;
                    Miracast_log(LOG_INFO, "%s: Miracast Plugin: startMiracastApp\n", AIRPLAY_APP_LOG);
                    for (int i = 0; i < mArgc; i++)
                    {
                        key = "argv" + std::to_string(i);
                        Miracast_log(LOG_INFO, "%s: Miracast Plugin: %s: %s\n", AIRPLAY_APP_LOG, key.c_str(), parameters.Get(key.c_str()).String().c_str());
                        mArgv[i] = strdup(parameters.Get(key.c_str()).String().c_str());
                    }
                    appInterface_startAirPlayApp(mArgc, mArgv);
                    err = Core::ERROR_NONE;
                }
                else
                {
                    Miracast_log(LOG_INFO, "%s: Miracast Plugin: No argc parameter", AIRPLAY_APP_LOG);
                    err = Core::ERROR_GENERAL;
                }
                return err;
            }

            virtual uint32_t StopApp(const string &paramStr)
            {
                Miracast_log(LOG_INFO, "%s: Miracast Plugin: StopApp \n", AIRPLAY_APP_LOG);
                // Send notification to RA to deactivate Miracast Plugin
                _adminLock.Lock();
                appInterface_stopAirPlayApp();
                _adminLock.Unlock();
                return Core::ERROR_NONE;
            }
            virtual void Unregister(PluginHost::IStateControl::INotification *sink)
            {
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Unregister IStateControl::INotification \n", AIRPLAY_APP_LOG);
                printf("Miracast Plugin: Unregister IStateControl::INotification\n");
                fflush(stdout);
                _adminLock.Lock();
                std::list<PluginHost::IStateControl::INotification *>::iterator index(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink));
                // Make sure you do not unregister something you did not register !!!
                ASSERT(index != _stateControlClients.end());

                if (index != _stateControlClients.end())
                {
                    (*index)->Release();
                    _stateControlClients.erase(index);
                }
                printf("Miracast Plugin: Unregistered IStateControl::INotification: ControlClients count:%d\n", _stateControlClients.size());
                fflush(stdout);
                _adminLock.Unlock();
            }
            virtual void Register(PluginHost::IStateControl::INotification *sink)
            {
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Register IStateControl::INotification \n", AIRPLAY_APP_LOG);
                _adminLock.Lock();
                // Make sure a sink is not registered multiple times.
                ASSERT(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink) == _stateControlClients.end());
                _stateControlClients.push_back(sink);
                sink->AddRef();
                _adminLock.Unlock();
            }
            virtual void Register(Exchange::IMiracast::INotification *sink)
            {
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: Register IMiracast::INotification\n", AIRPLAY_APP_LOG);
                _adminLock.Lock();
                ASSERT(std::find(_miracastClients.begin(), _miracastClients.end(), sink) == _miracastClients.end());
                _miracastClients.push_back(sink);
                sink->AddRef();
                _adminLock.Unlock();
            }
            virtual void Unregister(Exchange::IMiracast::INotification *sink)
            {
                Miracast_log(LOG_DEBUG, "%s:Miracast Plugin: Unregister IMiracast::INotification\n", AIRPLAY_APP_LOG);
                _adminLock.Lock();
                std::list<Exchange::IMiracast::INotification *>::iterator index(std::find(_miracastClients.begin(), _miracastClients.end(), sink));
                ASSERT(index != _miracastClients.end());
                if (index != _miracastClients.end())
                {
                    (*index)->Release();
                    _miracastClients.erase(index);
                }
                printf("Miracast Plugin: Unregistered IMiracast::INotification: ControlClients count:%d\n", _miracastClients.size());
                fflush(stdout);
                _adminLock.Unlock();
            }

            virtual PluginHost::IStateControl::state State() const
            {
                return (_state);
            }

            void StateChange(const PluginHost::IStateControl::state newState)
            {
                _adminLock.Lock();

                _state = newState;
                std::list<PluginHost::IStateControl::INotification *>::iterator index(_stateControlClients.begin());
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: StateChange() method with IStateControl:%d\n", AIRPLAY_APP_LOG, _stateControlClients.size());
                while (index != _stateControlClients.end())
                {
                    (*index)->StateChange(newState);
                    index++;
                }

                _adminLock.Unlock();
            }

            void StateChange(const Exchange::IMiracast::state newState)
            {
                _adminLock.Lock();

                std::list<Exchange::IMiracast::INotification *>::iterator index(_miracastClients.begin());
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: StateChange() method with INotification:%d \n", AIRPLAY_APP_LOG, _miracastClients.size());
                while (index != _miracastClients.end())
                {
                    (*index)->StateChange(newState);
                    index++;
                }

                _adminLock.Unlock();
            }

            virtual uint32_t Request(const PluginHost::IStateControl::command command)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;
                _adminLock.Lock();
                if (State() == PluginHost::IStateControl::UNINITIALIZED || State() == PluginHost::IStateControl::EXITED)
                {
                    Miracast_log(LOG_INFO, "%s: Miracast Plugin state is UNINITIALIZED or EXITED.\n", AIRPLAY_APP_LOG);
                    ASSERT(false);
                }
                else
                {
                    switch (command)
                    {
                    case PluginHost::IStateControl::SUSPEND:
                        if (State() == PluginHost::IStateControl::RESUMED)
                        {
                            Miracast_log(LOG_INFO, "%s: Miracast Plugin current state is Resumed.\n", AIRPLAY_APP_LOG);
                            SetSuspended(true);
                            result = Core::ERROR_NONE;
                        }
                        else
                        {
                            /*
                             * second time SUSPEND request is not error if plugin is
                             * already suspended
                             */
                            Miracast_log(LOG_INFO, "%s: Miracast suspend request in progress\n", AIRPLAY_APP_LOG);
                            result = Core::ERROR_NONE;
                        }
                        break;
                    case PluginHost::IStateControl::RESUME:
                        if (State() == PluginHost::IStateControl::SUSPENDED)
                        {
                            Miracast_log(LOG_INFO, "%s: Miracast Plugin current state is Suspended.\n", AIRPLAY_APP_LOG);
                            SetSuspended(false);
                            result = Core::ERROR_NONE;
                        }
                        else
                        {
                            /*
                             * second time RESUME request is not error if plugin is
                             * already resumed
                             */
                            Miracast_log(LOG_INFO, "%s: Miracast resume request in progress\n", AIRPLAY_APP_LOG);
                            result = Core::ERROR_NONE;
                        }
                        break;
                    default:
                        Miracast_log(LOG_ERR, "%s: Miracast %s Unsupported request\n", AIRPLAY_APP_LOG, __func__);
                        break;
                    }
                }
                _adminLock.Unlock();
                return result;
            }

            void SignalHandler(int sig)
            {
                Miracast_log(LOG_DEBUG, "%s:[SIG_HANDLER] Signal=%d caught by SignalHandler from callback!\n", AIRPLAY_APP_LOG, sig);

                // allow signal to be processed normally for correct core dump
                //  syslog(LOG_NOTICE,"Restore breakpad signal handlers\n");
                for (int i = 0; i < kNumHandledSignals; ++i)
                {
                    if (sigaction(kExceptionSignals[i], &old_handlers[i], NULL) == -1)
                    {
                        perror("sigaction");
                        signal(kExceptionSignals[i], SIG_DFL);
                    }
                }

                if (sig == SIGTERM || sig == SIGINT || sig == SIGALRM)
                {
                    Miracast_log(LOG_DEBUG, "%s: [SIG_HANDLER] SIGTERM and SIGINT will be considerd normal shutdowns, shutting down immediately\n", AIRPLAY_APP_LOG);
                    _exit(0);
                }
            }

        private:
            class Config : public Core::JSON::Container
            {
            private:
                Config(const Config &);
                Config &operator=(const Config &);

            public:
                Core::JSON::String ProvisioningParams;
                Core::JSON::String ClientIdentifier;
                Config()
                    : Core::JSON::Container(), ProvisioningParams(), ClientIdentifier()

                {
                    Add(_T("provisioningParams"), &ProvisioningParams);
                    Add(_T("clientidentifier"), &ClientIdentifier);
                }
                ~Config() {}
            };
            Config _config;

            void RegisterSignalHandler()
            {
                Miracast_log(LOG_DEBUG, "%s: Registering SignalHandler() \n", AIRPLAY_APP_LOG);
                // Backup breakpad signal handlers in old_handlers
                for (int i = 0; i < kNumHandledSignals; ++i)
                {
                    if (sigaction(kExceptionSignals[i], NULL, &old_handlers[i]) == -1)
                    {
                        perror("sigaction");
                        return;
                    }
                }

                struct sigaction act;
                bool signal_attached = true;
                sigset_t mask;
                sigset_t orig_mask;
                memset(&act, 0, sizeof(act));
                act.sa_handler = SignalHandler_callback;

                // Override signals with the Miracast thunder plugin sepcific signal handler
                for (int i = 0; i < kNumHandledSignals; ++i)
                {
                    if (sigaction(kExceptionSignals[i], &act, NULL))
                    {
                        perror("sigaction");
                        signal_attached = false;
                    }
                }

                // catch SIGINT and SIGTERM for proper shutdown

                if (sigaction(SIGTERM, &act, 0))
                {
                    perror("sigaction");
                    signal_attached = false;
                }

                if (sigaction(SIGINT, &act, 0))
                {
                    perror("sigaction");
                    signal_attached = false;
                }

                // handle custom signals so we can report better shutdown reasons to Miracast
                // SIGALRM will be used to notify that the app shutdown because of low resources
                if (sigaction(SIGALRM, &act, 0))
                {
                    perror("sigaction");
                    signal_attached = false;
                }

                signal(SIGPIPE, SIG_IGN);
                sigemptyset(&mask);
                sigaddset(&mask, SIGTERM);

                // #ifdef ENABLE_BREAKPAD
                for (int i = 0; i < kNumHandledSignals; ++i)
                {
                    sigaddset(&mask, kExceptionSignals[i]);
                }
                // #endif

                if (false == signal_attached)
                {
                    Miracast_log(LOG_DEBUG, "%s: [SIG_HANDLER] Cannot attach some signal... exiting Miracast Plugin\n", AIRPLAY_APP_LOG);
                    return;
                }

                if (sigprocmask(SIG_UNBLOCK, &mask, &orig_mask) < 0)
                {
                    Miracast_log(LOG_DEBUG, "%s: sigprocmask() failed to unblock SIGTERM\n", AIRPLAY_APP_LOG);
                    return;
                }

                Miracast_log(LOG_DEBUG, "%s: [SIG_HANDLER] SIGTERM, SIGINT, and SIGALRM handlers installed...\n", AIRPLAY_APP_LOG);
            }

            virtual uint32_t Configure(PluginHost::IShell *service)
            {
                string value;
                uint32_t result = Core::ERROR_NONE;
                JsonObject MiracastAppConfiguration;
                int argc = 0;

                _config.FromString(service->ConfigLine());

                MiracastAppConfiguration.FromString(service->ConfigLine().c_str());
                if (MiracastAppConfiguration.HasLabel("argc"))
                {
                    argc = std::stoi(MiracastAppConfiguration["argc"].Value());
                }
                if (MiracastAppConfiguration.HasLabel("launchAppInForeground"))
                {
                    if (MiracastAppConfiguration["launchAppInForeground"].Boolean() == true)
                    {
                        _state = PluginHost::IStateControl::SUSPENDED;
                    }
                    else
                    {
                        _state = PluginHost::IStateControl::RESUMED;
                    }
                    Miracast_log(LOG_INFO, "%s: %s: launchAppInForeground set as %d _state %d\n", AIRPLAY_APP_LOG, __func__, MiracastAppConfiguration["launchAppInForeground"].Boolean(), _state);
                }

                Miracast_log(LOG_DEBUG, "%s: Miracast %s: argc=%d \n", AIRPLAY_APP_LOG, __func__, argc);
                /*
                 * If subsequent launch command is called, then we may not have any args
                 * No need to configure again in such case
                 */
                if (argc <= 0)
                {
                    Miracast_log(LOG_DEBUG, "%s: Miracast Plugin already configured, skip config again\n", AIRPLAY_APP_LOG);
                    return Core::ERROR_NONE;
                }

                if (_config.ClientIdentifier.IsSet() == true)
                {
                    Core::SystemInfo::SetEnvironment(_T("WAYLAND_DISPLAY"), _config.ClientIdentifier.Value());
                    string value(service->Callsign() + ',' + _config.ClientIdentifier.Value());
                    Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value);
                }
                else
                {
                    Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign());
                }

                for (auto i = 0; i < argc; i++)
                {
                    std::string argv = "argv" + std::to_string(i);
                    std::string config_option;
                    config_option = MiracastAppConfiguration[argv.c_str()].Value();
                    _options.Add(_T(config_option.c_str()));
                }

                RegisterSignalHandler();
                Run();
                return Core::ERROR_NONE;
            }

            virtual uint32_t Worker()
            {
                std::string envOverride;

                TRACE(Trace::Information, (_T("Starting Miracast")));

                // Set JSON options
                //  create parameters and count number of options from size of each param
                uint16_t size = _options.BlockSize();
                void *parameters = ::malloc(size);
                int argc = _options.Block(parameters, size);
                // delete plugin specific cache from file system

                // This thread will run after Configure
                // main function to start app and block this thread
                // for us this could be startMiracastApp or jump to miracast daemon
                // uint32_t result = gibbon_main(argc, reinterpret_cast<char**>(parameters));

                uint32_t result = appInterface_startAirPlayApp(argc, reinterpret_cast<char **>(parameters));

                Block();

                // uint32 result = 0;
                //  Exit Plugin if not blocked anymore
                Miracast_log(LOG_INFO, "%s: Miracast Plugin: Miracast App stopped\n", AIRPLAY_APP_LOG);
                fflush(stdout);
                Exit(result);

                return (Core::infinite);
            }
            void Exit(const uint32_t exitCode)
            {
                Miracast_log(LOG_DEBUG, "%s: Miracast Plugin: exitCode=%d\n", AIRPLAY_APP_LOG, exitCode);
                fflush(stdout);
                _adminLock.Lock();
                if (exitCode == 0)
                {
                    // Application forced to shutdown
                    _isAppForceStopped = true;
                }
                if (!_miracastClients.empty())
                {
                    Miracast_log(LOG_DEBUG, "%s:_miracastClients list not empty\n", AIRPLAY_APP_LOG);
                    std::list<Exchange::IMiracast::INotification *>::iterator index(_miracastClients.begin());
                    while (index != _miracastClients.end())
                    {
                        (*index)->Exit(exitCode);
                        index++;
                    }
                }
                _adminLock.Unlock();
            }

        public:
            BEGIN_INTERFACE_MAP(MiracastImpl)
            INTERFACE_ENTRY(Exchange::IMiracast)
            INTERFACE_ENTRY(PluginHost::IStateControl)
            END_INTERFACE_MAP

        protected:
            Core::Process::Options _options;

        private:
            mutable Core::CriticalSection _adminLock;
            const char **mArgv;
            int mArgc;
            MiracastImpl(const MiracastImpl &) = delete;
            MiracastImpl &operator=(const MiracastImpl &) = delete;
            PluginHost::IStateControl::state _state;
            bool _startSuspended;
            std::list<Exchange::IMiracast::INotification *> _miracastClients;
            std::list<PluginHost::IStateControl::INotification *> _stateControlClients;
            // NotificationSink _sink;
            bool _resumePending;
            bool _compliant;
            bool _isPlaying;
            bool _isAppForceStopped;
        };

        SERVICE_REGISTRATION(MiracastImpl, 1, 0);

    } // namespace Plugin
} // namespace WPEFramework

void SignalHandler_callback(int sig)
{
    Miracast_log(LOG_DEBUG, "%s: [SIG_HANDLER]  Signal=%d caught by SignalHandler_callback\n", AIRPLAY_APP_LOG, sig);
    WPEFramework::Plugin::implementation->SignalHandler(sig);
}