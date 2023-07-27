#include "allplayer.h"
#include "allplayer_manager.h"

AP_API int32_t	ap_lib_init(char* logPath, int32_t logLevel, int thdlimit)
{
	return CAllplayerManager::GetInstance()->allplayerInit(logPath, logLevel, thdlimit);
}

AP_API int32_t	ap_lib_excute(long lBusiness, char* pCharBusiness, char* pResultData, int len)
{
	return CAllplayerManager::GetInstance()->excuteBussiness(lBusiness, pCharBusiness, pResultData, len);
}

AP_API int32_t	ap_lib_unit()
{
	return CAllplayerManager::GetInstance()->allplayerRelease();
}

AP_API void	ap_lib_reg_status_callback(allplay_status_callback pCallBack)
{
	CAllplayerManager::GetInstance()->registerStatusCallback(pCallBack);
}

AP_API void	ap_lib_reg_data_callback(allplay_progress_callback pCallBack)
{
	CAllplayerManager::GetInstance()->registerDataCallback(pCallBack);
}
