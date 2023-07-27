#ifndef __AS_TIME_H_INCLUDE
#define __AS_TIME_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"

void       as_sleep(uint32_t ulMs );
uint32_t   as_get_ticks (void);
uint32_t   as_get_cur_msecond(void);
void       as_strftime(char * pszTimeStr, uint32_t ulLens, char* pszFormat, time_t ulTime);
time_t     as_str2time(const char *pStr);//YYYYMMDDHHmmSS
struct tm* as_Localtime(time_t* ulTime);
time_t     as_stdstr2time(const char *pStr);//YYYY-MM-DD HH:mm:SS
char*      as_time2manstr(char *pDestBuf, int32_t nbuflen, const time_t t);
char*      as_time2str(char *pDestBuf, int32_t nbuflen, const time_t t);
char*      as_time2stdstr(char *pDestBuf, int32_t nbuflen, const time_t t);
char*      as_time2stdstrwith_t(char *pDestBuf, int32_t nbuflen, const time_t t);
char*      as_time2with_str(char *pDestBuf, int32_t nbuflen, const time_t *calptr);
time_t     as_hh24miss2second(const char *strTime);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // __AS_TIME_H_INCLUDE

