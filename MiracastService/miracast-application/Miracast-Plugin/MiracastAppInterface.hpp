#ifndef MiracastAppInterface_hpp
#define MiracastAppInterface_hpp

extern int app_exit_err_code;
enum Miracast_engine_state {
    Miracast_STOPPED,
    Miracast_STARTED,
    Miracast_RUNNING,
    Miracast_BACKGROUNDING,
    Miracast_BACKGROUNDED,
    Miracast_RESUMING,
};

int appInterface_startMiracastApp(int argc, const char **argv);
void appInterface_stopMiracastApp();
void appInterface_destroyAppInstance();
int appInterface_getAppState();
void appInterface_setAppVisibility(bool visible);
bool isAppExitRequested();


#endif /*MiracastAppInterface_hpp*/