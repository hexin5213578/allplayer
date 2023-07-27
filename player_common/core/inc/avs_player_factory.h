#ifndef __AVS_PLAYER_FACTORY_H__
#define __AVS_PLAYER_FACTORY_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mutex>
#include <memory>
#include "avs_player_common.h"
#include "avs_loop_thread.h"

typedef void (*allplay_status_callback)(long id, long type, long status, const char* info);
typedef void (*allplay_progress_callback)(long id, long type, long current, long total);

class AvsPlayer;

class avs_player_factory
{
public:
    static avs_player_factory* GetInstance() {
        static avs_player_factory objAvsPlayerFactory;
        return &objAvsPlayerFactory;
    }
    
	int32_t init(uint32_t ulRecvThread = STREAM_RECV_THREAD_DEFAULT, uint32_t ulMaxPlayer = PLAYER_MAX_DEFAULT, const char* path = "", int32_t loglevel = 0);
    void    release();

	void setDecoderThreadLimit(int limit) { m_thdLimit = limit; }
	int getDecoderThreadLimit() { return m_thdLimit; }

	inline std::map<long, AvsPlayer*>& getPlayerMap() {
		return m_mapVideoService;
	}

    int32_t startLog(int32_t lLevel,const char* logfile);
    void    stopLog();

	//根据业务ID查找avs_player对象
	AvsPlayer* findPlayerByBusinessID(long lBusinessID);

	AvsPlayer* findPlayerByHwnd(void* hwnd);

	void setSkipNokey(bool skip);

	/**
	 * 注册状态回调
	 * \param pCallBack
	 */
	void registerStatusCallback(allplay_status_callback pCallBack);

	void reportStatus(long lBusinessID, long busType, long status, const char* info);

	void registerDataCallback(allplay_progress_callback pCallBack);

	void reportFrameInfo(long lBusinessID, long busType, long cur, long total);

	void sendStatusMsg(long lBusinessID, long busType, long status, std::string description);

	void sendDataMsg(long lBusinessID, long busType, long cur, long total);

	//创建avs_player对象
	AvsPlayer* createPlayer(BizParams& params);
	//销毁avs_player对象
	void deletePlayer(long lBusinessID);

protected:
    static void ff_log_callback_function(void *ptr, int level, const char *fmt, va_list vl);
    static void ff_log_callback_null(void *ptr, int level, const char *fmt, va_list vl);
    static void mk_log_callback_function(const char* szFileName, int32_t lLine,int32_t lLevel, const char* format,va_list argp);

protected:
    avs_player_factory();
    virtual ~avs_player_factory();

private:
	std::map<long, AvsPlayer*>				m_mapVideoService;
    allplay_status_callback					m_statusCb;		//状态回调
	allplay_progress_callback				m_dataCb;
    std::mutex								m_statusMtx;
	std::mutex								m_dataMtx;
	std::shared_ptr<avs_loop_thread>		m_pMsgCbThread;
	bool									m_skipNokeyPkt;
	int										m_thdLimit;
};

#endif /* __AVS_PLAYER_FACTORY_H__ */
