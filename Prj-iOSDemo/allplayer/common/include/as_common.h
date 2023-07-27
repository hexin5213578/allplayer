#ifndef __AS_MEDIA_COMMON_H__
#define __AS_MEDIA_COMMON_H__
#include "as_config.h"
#include "as_thread.h"
#include "as_mutex.h"
#include "as_event.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#if AS_APP_OS == AS_OS_WIN32
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
#endif


enum AS_ERROR_CODE {
    AS_ERROR_CODE_ABORT       = -6,
    AS_ERROR_CODE_DECLINED    = -5,
    AS_ERROR_CODE_DONE        = -4,
    AS_ERROR_CODE_BUSY        = -3,
    AS_ERROR_CODE_AGAIN       = -2,
    AS_ERROR_CODE_FAIL        = -1,
    AS_ERROR_CODE_OK          = 0x00,
    AS_ERROR_CODE_MEM         = 0x01,
    AS_ERROR_CODE_PARAM       = 0x02,
    AS_ERROR_CODE_SYS         = 0x03,
    AS_ERROR_CODE_TIMEOUT     = 0x04,
    AS_ERROR_CODE_QUE_EMPTY   = 0x05,
    AS_ERROR_CODE_QUE_LEN     = 0x06,
    AS_ERROR_CODE_MAX
};


#endif /*__AS_MEDIA_COMMON_H__*/

