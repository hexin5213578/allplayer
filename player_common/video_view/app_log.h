#ifndef XPLAY_XLOG_H
#define XPLAY_XLOG_H

#include "as_config.h"
#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include <android/log.h>
#define XLOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"XPlay",__VA_ARGS__)
#define XLOGI(...) __android_log_print(ANDROID_LOG_INFO,"XPlay",__VA_ARGS__)
#define XLOGE(...) __android_log_print(ANDROID_LOG_ERROR,"XPlay",__VA_ARGS__)
#elif ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include <syslog.h>
#define XLOGD(...) syslog(LOG_DEBUG,__VA_ARGS__)
#define XLOGI(...) syslog(LOG_INFO,__VA_ARGS__)
#define XLOGE(...) syslog(LOG_ERR,__VA_ARGS__)
#endif


#endif //XPLAY_XLOG_H
