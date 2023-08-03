/*
    Copyright (C) 2021 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
*/
#ifndef MiracastLogging_hpp
#define MiracastLogging_hpp
#define kLoggingFacilityAppEngine "Miracast.Engine"
#define MIRACAST_APP_LOG "MiracastApp"
extern int Level_Limit;
#include <string>
#include <syslog.h>

//TODO: Flush out logging
namespace Miracast {
namespace Logging {
#define Miracast_log(level,...)                          \
    do                             \
    {                            \
        if(level <= Level_Limit)        \
        {                        \
                syslog(level,__VA_ARGS__);            \
                }                        \
    }while(0)

void setLogLevel();
}
}
#endif /* MiracastLogging_hpp */
