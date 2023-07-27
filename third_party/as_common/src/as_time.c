#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_time.h"


#if AS_APP_OS == AS_OS_WIN32
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment( lib,"winmm.lib" )
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <time.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#endif

#include <errno.h>

static uint32_t g_ulSysStart = 0 ;

typedef struct _AS_TIMESTR_STRUCT
{
    uint8_t Year[4];
    uint8_t Month[2];
    uint8_t Day[2];
    uint8_t Hour[2];
    uint8_t Minute[2];
    uint8_t Second[2];
}AS_TIMESTR_STRUCT, *PAS_TIMESTR_STRUCT;

typedef struct _EDGE_STR_HH24MISS_STRUCT
{
    char Hour[2];
    char Min[2];
    char Sec[2];
}AS_STR_HH24MISS_STRUCT, *PAS_STR_HH24MISS_STRUCT;


uint32_t as_get_ticks (void)
{
    uint32_t ticks = 0 ;

#if AS_APP_OS ==  AS_OS_WIN32
    ticks = (uint32_t)(timeGetTime()/1000);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks=(uint32_t)now.tv_sec;
#endif

    return( ticks );
}

uint32_t as_get_cur_msecond(void)
{
    ULONG ticks = 0;

#if AS_APP_OS ==  AS_OS_WIN32
    ticks = timeGetTime();
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks = now.tv_sec*1000+now.tv_usec/1000;
#endif

    return(ticks);
}

/*1000 = 1second*/
void  as_sleep(uint32_t ulMs)
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    usleep( ulMs*1000 );
#elif AS_APP_OS == AS_OS_WIN32
    Sleep(ulMs);
#endif

return ;
}

void  as_strftime(char * pszTimeStr, uint32_t ulLens, char* pszFormat, time_t ulTime)
{
    struct tm* tmv;
    time_t uTime = (time_t)ulTime;
    tmv = (struct tm*)localtime(&uTime);

    strftime(pszTimeStr, ulLens, pszFormat, tmv);
    return;
}

struct tm* as_Localtime(time_t* ulTime)
{
    return (struct tm*)localtime(ulTime);/*0~6���� ���յ�����*/
}


//YYYYMMDDHHmmSS
time_t as_str2time(const char *pStr)
{
    struct tm tmvalue;

    (void)memset(&tmvalue, 0, sizeof(tmvalue));

    const char *pch = pStr;
    char tmpstr[8];
    memcpy(tmpstr, pch, 4);
    tmpstr[4] = '\0';
    tmvalue.tm_year = atoi(tmpstr) - 1900;
    pch += 4;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mon = atoi(tmpstr) - 1;
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mday = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_hour = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_min = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_sec = atoi(tmpstr);

    return mktime(&tmvalue);
}

time_t as_stdstr2time(const char *pStr)
{
    struct tm tmvalue;

    (void)memset(&tmvalue, 0, sizeof(tmvalue));

    const char *pch = pStr;
    char tmpstr[8];
    memcpy(tmpstr, pch, 4);
    tmpstr[4] = '\0';
    tmvalue.tm_year = atoi(tmpstr) - 1900;
    pch += 5;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mon = atoi(tmpstr) - 1;
    pch += 3;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mday = atoi(tmpstr);
    pch += 3;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_hour = atoi(tmpstr);
    pch += 3;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_min = atoi(tmpstr);
    pch += 3;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_sec = atoi(tmpstr);

    return mktime(&tmvalue);
}


char* as_time2manstr(char *pDestBuf, int32_t nbuflen, const time_t t)
{
    struct tm tmv;
#if AS_APP_OS == AS_OS_WIN32
    (void)localtime_s( &tmv ,&t);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    (void)localtime_r(&t, &tmv );
#endif
    (void)snprintf(   pDestBuf,
                        (size_t)nbuflen,
                        "%04d-%02d-%02d %02d:%02d:%02d",
                        tmv.tm_year + 1900,
                        tmv.tm_mon + 1,
                        tmv.tm_mday,
                        tmv.tm_hour,
                        tmv.tm_min,
                        tmv.tm_sec);

    return pDestBuf;
}

char *as_time2str(char *pDestBuf, int32_t nbuflen, const time_t t)
{
    struct tm tmv;

#if AS_APP_OS == AS_OS_WIN32
    (void)localtime_s( &tmv ,&t);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    (void)localtime_r(&t, &tmv );
#endif
    (void)snprintf(pDestBuf,
                     (size_t)nbuflen,
                     "%04d%02d%02d%02d%02d%02d",
                     tmv.tm_year + 1900,
                     tmv.tm_mon + 1,
                     tmv.tm_mday,
                     tmv.tm_hour,
                     tmv.tm_min,
                     tmv.tm_sec);

    return pDestBuf;
}

char *as_time2stdstr(char *pDestBuf, int32_t nbuflen, const time_t t)
{
    struct tm tmv;

#if AS_APP_OS == AS_OS_WIN32
    (void)localtime_s( &tmv ,&t);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    (void)localtime_r(&t, &tmv );
#endif
    (void)snprintf(pDestBuf,
                     (size_t)nbuflen,
                     "%04d-%02d-%02d %02d:%02d:%02d",
                     tmv.tm_year + 1900,
                     tmv.tm_mon + 1,
                     tmv.tm_mday,
                     tmv.tm_hour,
                     tmv.tm_min,
                     tmv.tm_sec);

    return pDestBuf;
}
char *as_time2stdstrwith_t(char *pDestBuf, int32_t nbuflen, const time_t t)
{
    struct tm tmv;

#if AS_APP_OS == AS_OS_WIN32
    (void)localtime_s( &tmv ,&t);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    (void)localtime_r(&t, &tmv );
#endif
    (void)snprintf(pDestBuf,
                     (size_t)nbuflen,
                     "%04d-%02d-%02dT%02d:%02d:%02d",
                     tmv.tm_year + 1900,
                     tmv.tm_mon + 1,
                     tmv.tm_mday,
                     tmv.tm_hour,
                     tmv.tm_min,
                     tmv.tm_sec);

    return pDestBuf;
}


char *as_time2with_str(char *pDestBuf, int32_t nbuflen, const time_t *calptr)
{
    struct tm tmv;

#if AS_APP_OS == AS_OS_WIN32
    (void)localtime_s( &tmv ,calptr);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    (void)localtime_r(calptr, &tmv );
#endif
    (void)snprintf(   pDestBuf,
                        (size_t)nbuflen,
                        "%04d_%02d_%02d_%02d_%02d_%02d",
                        tmv.tm_year + 1900,
                        tmv.tm_mon + 1,
                        tmv.tm_mday,
                        tmv.tm_hour,
                        tmv.tm_min,
                        tmv.tm_sec);

    return pDestBuf;
}
time_t as_hh24miss2second(const char *strTime)
{
    if (NULL == strTime)
    {
        return 0;
    }

    const int32_t hh24missLen = 6;
    char buf[24];
    (void)memcpy(buf, strTime, hh24missLen);
    buf[hh24missLen] = '\0';
    time_t ret = atoi(((PAS_STR_HH24MISS_STRUCT)buf)->Sec);  //seconds

    if ((0 > ret) || (60 <= ret))
    {
        return 0;
    }

    {
        ((PAS_STR_HH24MISS_STRUCT)buf)->Sec[0] = '\0';
        time_t tmMinute = atoi(((PAS_STR_HH24MISS_STRUCT)buf)->Min);

        if ((0 > tmMinute) || (60 <= tmMinute))
        {
            return 0;
        }

        ret += (tmMinute * 60);

        ((PAS_STR_HH24MISS_STRUCT)buf)->Min[0] = '\0';
        time_t tmHour = atoi(((PAS_STR_HH24MISS_STRUCT)buf)->Hour);  //Сʱ

        if ((0 > tmHour) || (24 < tmHour))
        {
            return 0;
        }
        ret += (tmHour * 60 * 60);
    }
    return ret;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */



