#ifndef ALLPLAYER_MANAGER_H
#define ALLPLAYER_MANAGER_H

#include "avs_player_common.h"

#include <cstdint>
#include <set>

#include "allplayer.h"

#if ANDROID
#include <android/native_window.h>
#endif

class CAllplayerManager
{

public:
	static CAllplayerManager* GetInstance();

	int32_t allplayerInit(char* pLogPath, int32_t iLogLevel, int thdLimits);
	int32_t allplayerRelease();
	int32_t excuteBussiness(long lBusinessID, char* pBusinessParameters, char* pResultData = nullptr, int len = 0);

	/**
	 * .
	 * 注册状态回调
	 * \param pCallBack
	 */
	void registerStatusCallback(allplay_status_callback pCallBack);

	void registerDataCallback(allplay_progress_callback pCallBack);

#if ANDROID
	ANativeWindow* getNativeWindow(long lBusinessID);

	void setNativeWindow(long lBusinessID, ANativeWindow* win);

	void changeSurface(long lBusinessID, ANativeWindow* win, int with, int height);

	void clearSurface(long lBusinessID);

	void collectAudioFrame(unsigned char* data, int dataSize);

	void stopAudioRecord();

#endif

protected:
	CAllplayerManager();
	~CAllplayerManager();

private:
	int32_t parseBussinessJson(long lBusinessID, char* pBusinessJson, BizParams& info, char* pResultData, int len);
	
	int32_t excuteBussiness(long lBusinessID, BizParams& info, char* pResultData, int len);

	int32_t	excuteInternal(ExcuteInfo& excute, char* pResultData = 0, int len = 0);

	std::set<void*>		hwnds_;

#if ANDROID
	std::map<long, ANativeWindow*> m_windowsMap;
#endif
};


#endif  //ALLPLAYER_WINDOWS_H