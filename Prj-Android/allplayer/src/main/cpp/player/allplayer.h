#ifndef __LIB_ALL_PLAYER_H__
#define __LIB_ALL_PLAYER_H__

#ifdef WIN32
#ifdef LIBALLPLAYER_EXPORTS
#define AP_API __declspec(dllexport)
#else
#define AP_API __declspec(dllimport)
#endif

#else
    #define AP_API
#endif


//#include <as.h>
#include <stdint.h>
#include <stdarg.h>

typedef void (*allplay_status_callback)(long lBusiness, long busType, long status, const char* info);

typedef void (*allplay_progress_callback)(long lBusiness, long busType, long current, long total);

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
    /* init the media  library */
#ifdef _WIN64
	AP_API int32_t   ap_lib_init(char* path, int32_t level, int thdlimit = 4);
#else 
	AP_API int32_t   ap_lib_init(char* path, int32_t level, int thdlimit = 0);
#endif 
	AP_API int32_t   ap_lib_excute(long lBusiness, char* pCharBusiness, char* pResultData = nullptr, int len = 0);
	AP_API int32_t   ap_lib_unit();
	AP_API void		 ap_lib_reg_status_callback(allplay_status_callback pCallBack);
	AP_API void		 ap_lib_reg_data_callback(allplay_progress_callback pCallBack);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /*__LIB_MEDIA_RTSP_H__*/
