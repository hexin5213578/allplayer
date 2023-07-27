#ifndef __AS_MEDIA_KENERL_CONFIG_H__
#define __AS_MEDIA_KENERL_CONFIG_H__
#include <stdio.h>

/* unix */
#define   AS_OS_UNIX                0x0100

#define   AS_OS_LINUX               0x0101
#define   AS_OS_MAC                 0x0102
#define   AS_OS_IOS                 0x0103
#define   AS_OS_ANDROID             0x0104
/* windows */
#define   AS_OS_WIN                 0x0200

#define   AS_OS_WIN32               0x0201
#define   AS_OS_WIN64               0x0202



#define   AS_BIG_ENDIAN                     0
#define   AS_LITTLE_ENDIAN                  1


#ifdef ENV_LINUX
#define   AS_APP_OS                     AS_OS_LINUX
#define   AS_BYTE_ORDER                 AS_LITTLE_ENDIAN
#endif

#ifdef WIN32
#define   AS_APP_OS                     AS_OS_WIN32
#define   AS_BYTE_ORDER                 AS_LITTLE_ENDIAN

#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef strcasecmp
#define strcasecmp stricmp
#endif
#ifndef strncasecmp
#define strncasecmp strnicmp
#endif
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif

#include <Ws2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"ws2_32.lib")

#endif

#ifdef OS_ANDROID
#define   AS_APP_OS                     AS_OS_ANDROID
#define   AS_BYTE_ORDER                 AS_LITTLE_ENDIAN
#endif

#ifdef __APPLE__

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#define   AS_APP_OS                     AS_OS_IOS
#elif TARGET_OS_MAC
#define   AS_APP_OS                     AS_OS_MAC
#endif

#define   AS_BYTE_ORDER                 AS_LITTLE_ENDIAN
#endif

#endif /*__AS_MEDIA_KENERL_CONFIG_H__*/