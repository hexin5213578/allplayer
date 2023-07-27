#ifndef ALLPLAYER_MANAGER_H
#define ALLPLAYER_MANAGER_H

#include <cstdint>
//#include "avs_player_common_win.h"
#include "avs_allplayer_common_ios.h"
#include "avs_egress.h"

//#include "libAllPlayer.h"
typedef void (*allplay_status_callback)(long lBusiness, long busType, long status, const char* info);

typedef void (*allplay_progress_callback)(long lBusiness, long busType, long current, long total);

class CAllplayerManager
{

public:
	static CAllplayerManager* GetInstance();

	int32_t allplayerInit(char* pLogPath, int32_t iLogLevel);
	int32_t allplayerRelease();
	int32_t excuteBusiness(long lBusinessID, char* pBusinessParameters);
	int32_t excuteBusiness(long lBusinessID, BusinessInfoStruct& info);

	/**
	 * .
	 * 注册状态回调
	 * \param pCallBack
	 */
	void registerStatusCallback(allplay_status_callback pCallBack);

	void registerDataCallback(allplay_progress_callback pCallBack);
	
protected:
	CAllplayerManager();
	~CAllplayerManager();

private:
	int32_t parseBusinessJson(char* pBusinessJson, BusinessInfoStruct& info);

	avs_egress	m_concreteEgress;	//解码渲染+功能类,为了多平台适配
};


#endif  //ALLPLAYER_WINDOWS_H
