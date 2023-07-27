#ifndef AS_BASETYPE_H_INCLUDE
#define AS_BASETYPE_H_INCLUDE
#include <stdint.h>
#if ((AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX)
#ifndef SOCKET
typedef int SOCKET; 
#endif
#endif

#ifndef  ULONGLONG
#if ((AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX)
typedef  unsigned long long     ULONGLONG ;
#endif
#if (AS_APP_OS == AS_OS_WIN32)
typedef  unsigned __int64       ULONGLONG;
#endif
#endif

#ifndef  LONGLONG
#if ((AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX)
typedef  long long              LONGLONG;
#endif
#if (AS_APP_OS == AS_OS_WIN32)
typedef  __int64                LONGLONG;
#endif
#endif

#ifndef  ULONG
typedef  unsigned long          ULONG ;
#endif

#ifndef  LONG
typedef  long                   LONG;
#endif

#ifndef  uint16_t
typedef  unsigned short         uint16_t ;
#endif

#ifndef  SHORT
typedef  short                  SHORT ;
#endif

#ifndef  UCHAR
typedef  unsigned char          UCHAR ;
#endif

#ifndef  CHAR
typedef  char                   CHAR ;
#endif

#ifndef  VOID
typedef  void                   VOID ;
#endif

#ifndef  UINT
typedef  unsigned int          UINT ;
#endif

#ifndef  AS_BOOLEAN
typedef enum _AS_BOOLEAN
{
    AS_TRUE = 1,
    AS_FALSE = 0
}AS_BOOLEAN;
#endif


#ifndef  AS_NULL
#define  AS_NULL               NULL
#endif

#ifndef  AS_SUCCESS
#define  AS_SUCCESS             0
#endif

#ifndef  AS_FAIL
#define  AS_FAIL               -1
#endif

#if AS_BYTE_ORDER == AS_LITTLE_ENDIAN
#define AS_ntohl(x)            ((((x) & 0x000000ff)<<24)|(((x) & 0x0000ff00) << 8) |(((x) & 0x00ff0000)>>8) |(((x) & 0xff000000) >>24))
#define AS_htonl(x)            (AS_ntohl(x))
#else
#define AS_ntohl(x)            (x)
#define AS_htonl(x)            (AS_ntohl(x))
#endif

#if AS_BYTE_ORDER == AS_LITTLE_ENDIAN
#define AS_ntohs(x)            ((((x) & 0x00ff) << 8) |(((x) & 0xff00) >> 8))
#define AS_htons(x)            (AS_ntohs(x))
#else
#define AS_ntohs(x)          (x)
#define AS_htons(x)          (AS_ntohs(x))
#endif

#if AS_BYTE_ORDER == AS_LITTLE_ENDIAN
#define  AS_ntohll( x )        (((AS_ntohl( ((x) & 0xFFFFFFFF)))<< 32) | (AS_ntohl(((x)>>32)&0xFFFFFFFF)))
#define  AS_htonll( x )        (AS_ntohll(x))
#else
#define  AS_ntohll( x )        (x)
#define  AS_htonll( x )        (AS_ntohll(x))
#endif

#define QUEUE_MODE_NOWAIT  0
#define QUEUE_MODE_WAIT    1


#endif //AS_BASETYPE_H_INCLUDE
