
#include "avs_player_factory.h"
#include "avs_player.h"
#include "avs_rtsp_player.h"
#include "avs_mux_thread.h"
#include "avs_universal_player.h"
#include "crypto_player.h"
#include "avs_splice_player.h"
#include "as_config.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/log.h"
#include "libswresample/swresample.h"
};

#if ((AS_OS_IOS & AS_APP_OS) != AS_OS_IOS)
#include <SDL2/SDL.h>
#endif

#define AP_VERSION "ALLPLAYER_5.1.0"

void MsgCbHandler(unifyMsgStruct msg, void *context) {
    avs_player_factory *factory = static_cast<avs_player_factory *>(context);
    switch (msg.msgType) {
        case 0:
            factory->reportStatus(msg.bizId, msg.bizType, msg.status, msg.description.c_str());
            break;
        case 1:
            factory->reportFrameInfo(msg.bizId, msg.bizType, msg.current, msg.total);
            break;
        default:
            break;
    }
}

avs_player_factory::avs_player_factory() {
    m_statusCb = nullptr;
    m_dataCb = nullptr;
    m_pMsgCbThread = nullptr;
    m_skipNokeyPkt = false;
}

avs_player_factory::~avs_player_factory()  {
}

//根据业务ID查找avs_player对象
AvsPlayer *avs_player_factory::findPlayerByBusinessID(long lBusinessID) {
    AvsPlayer *pPlayer = nullptr;
    std::map<long, AvsPlayer*>::iterator playerIter = m_mapVideoService.find(lBusinessID);
    if (m_mapVideoService.end() != playerIter) {
        pPlayer = playerIter->second;
    }
    return pPlayer;
}

AvsPlayer* avs_player_factory::findPlayerByHwnd(void* hwnd)
{
    AvsPlayer* player = nullptr;
    for (auto iter: m_mapVideoService) {
        if (hwnd == iter.second->getHwnd()) {
            player = iter.second;
            break;
        }
    }
    return player;
}

void avs_player_factory::setSkipNokey(bool skip) {
    for (auto player : m_mapVideoService) {
        player.second->setSkipNokey(skip);
    }
    m_skipNokeyPkt = skip;
}

//创建avs_player对象
AvsPlayer *avs_player_factory::createPlayer(BizParams& params)  {
    auto id = params.BizId;
    AvsPlayer* player = findPlayerByBusinessID(id);
    if (!player) {
        switch (params.BizType) {
        case TYPE_REALVIDEO_START:
        case TYPE_NETRECORD_START:
            player = new AvsRtspPlayer(params);
            break;
        case TYPE_DOWNLOAD_START:
            player = new AvsMuxThread(params);
            break;
        case TYPE_URL_START:
            if (0 == params.PlayMode) {
                player = new AvsUniversalPlayer(params);
            }
            else if (1 == params.PlayMode) {
                player = new CryptoPlayer(params);
            }
            break;
        case TYPE_MULTI_REALVIDEO_START:
        case TYPE_MULTI_RECORD_START:
            player = new AvsSplicePlayer(params);
            break;
        default:
            break;
        }
    }

    m_mapVideoService.insert(make_pair(id, player));
    if (m_skipNokeyPkt) {
        player->setSkipNokey(m_skipNokeyPkt);
    }
    return player;
}

int32_t avs_player_factory::init(uint32_t ulRecvThread, uint32_t ulMaxPlayer, const char *path, int32_t level) 
{
    int32_t lRet = AS_ERROR_CODE_OK;
    startLog(level, path);

    /* 1.初始化FFMPEG 相关全局接口 */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif

#if ((AS_OS_IOS & AS_APP_OS) != AS_OS_IOS)
    /* 2.初始化SDL全局接口 */
    if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
        SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        auto err = SDL_GetError();
        AS_LOG(AS_LOG_ERROR, "Could not initialize SDL - %s\n", SDL_GetError());
        return AS_ERROR_CODE_FAIL;
    }
#endif
    
    /* 3,初始化libMediaServer全局接口 */
    lRet = mk_lib_init(ulRecvThread, mk_log_callback_function, ulMaxPlayer, RTP_PACKET_BUF_PER_CLIENT);
    if (AS_ERROR_CODE_OK != lRet) 
        return AS_ERROR_CODE_FAIL;

    if (!m_pMsgCbThread)  {
        m_pMsgCbThread = std::make_shared<avs_loop_thread>("msgCb", this, MsgCbHandler);
        m_pMsgCbThread->start();
    }

    AS_LOG(AS_LOG_CRITICAL, "AllPlayer Version %s, Level %d.", AP_VERSION, level);
    return lRet;
}

void avs_player_factory::release() 
{
    for (auto &bussiness : m_mapVideoService) {
        bussiness.second->stop();
        delete bussiness.second;
    }
    m_mapVideoService.clear();
    mk_lib_release();
    m_pMsgCbThread->stop();
    
#ifdef _WIN32
    SDL_CloseAudio();
    SDL_Quit();
#endif // _WIN32
    return;
}

int32_t avs_player_factory::startLog(int32_t lLevel, const char *logfile) {
    /* 设置 FFMPEG 日志回调函数 */
    av_log_set_flags(AV_LOG_SKIP_REPEATED);

    if (lLevel == AS_LOG_DEBUG) {
        av_log_set_level(AV_LOG_DEBUG);
    }
    else if (lLevel == AS_LOG_INFO) {
        av_log_set_level(AV_LOG_INFO);
    }
    else if (lLevel == AS_LOG_WARNING) {
        av_log_set_level(AV_LOG_WARNING);
    }
    else if (lLevel == AS_LOG_ERROR) {
        av_log_set_level(AV_LOG_ERROR);
    }
    else if (lLevel == AS_LOG_CRITICAL) {
        av_log_set_level(AV_LOG_FATAL);
    }
    else {
        av_log_set_level(AV_LOG_ERROR);
    }
    //av_log_set_callback(ff_log_callback_function);

    /* 启动日志模块 */
    ASSetLogLevel(lLevel);
    ASSetLogFilePathName(logfile);
    ASStartLog();
    return AS_ERROR_CODE_OK;
}

void avs_player_factory::stopLog() 
{
    /* 取消 FFMPEG 日志回调函数 */
    av_log_set_callback(ff_log_callback_null);
    ASStopLog();
    return;
}

void avs_player_factory::deletePlayer(long lBusinessID) 
{
    std::map<long, AvsPlayer*>::iterator it_map_business;
    it_map_business = m_mapVideoService.find(lBusinessID);
    if (it_map_business != m_mapVideoService.end()) 
    {
        AvsPlayer* pavs_player = it_map_business->second;
        pavs_player->stop();
        m_mapVideoService.erase(it_map_business);
        if (pavs_player) {
            delete pavs_player;
            pavs_player = NULL;
        }
    }
    return;
}

void avs_player_factory::ff_log_callback_function(void *ptr, int level, const char *fmt, va_list vl) {
    ASLogLevel enLevel = AS_LOG_DEBUG;

    if (av_log_get_level() < level)
        return;

    if (level == AV_LOG_TRACE) {
        enLevel = AS_LOG_DEBUG;
    } else if (level == AV_LOG_DEBUG) {
        enLevel = AS_LOG_DEBUG;
    } else if (level == AV_LOG_INFO) {
        enLevel = AS_LOG_INFO;
    } else if (level == AV_LOG_WARNING) {
        enLevel = AS_LOG_WARNING;
    } else if (level == AV_LOG_ERROR) {
        enLevel = AS_LOG_ERROR;
    } else if (level == AV_LOG_FATAL) {
        enLevel = AS_LOG_CRITICAL;
    } else {
        enLevel = AS_LOG_ERROR;
    }

    if (enLevel > AS_LOG_ERROR) {
        return;
    }

    va_list vl2;
    char line[PLAYER_LOG_MAX];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    //av_log_default_callback(ptr, level, fmt, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);
    AS_LOG(enLevel, "%s", line);
}


void avs_player_factory::registerStatusCallback(allplay_status_callback pCallBack)
{
    std::unique_lock<std::mutex> lck(m_statusMtx);
    m_statusCb = pCallBack;
}

void avs_player_factory::reportStatus(long lBusinessID, long busType, long status, const char *info) 
{
    std::unique_lock<std::mutex> lck(m_statusMtx);
    if (m_statusCb) {
        m_statusCb(lBusinessID, busType, status, info);
    }
}

void avs_player_factory::registerDataCallback(allplay_progress_callback pCallBack) 
{
    std::unique_lock<std::mutex> lck(m_dataMtx);
    m_dataCb = pCallBack;
}

void avs_player_factory::reportFrameInfo(long lBusinessID, long busType, long cur, long total) 
{
    std::unique_lock<std::mutex> lck(m_dataMtx);
    if (m_dataCb) {
        m_dataCb(lBusinessID, busType, cur, total);
    }
}

void avs_player_factory::sendStatusMsg(long id, long type, long status, std::string description)
{
    if (m_pMsgCbThread) {
        unifyMsgStruct msg;
        msg.msgType = 0;
        msg.bizId = id;
        msg.bizType = type;
        msg.status = status;
        msg.description = description;
        m_pMsgCbThread->sendMessage(msg);
    }
#ifdef _DEBUG
	AS_LOG(AS_LOG_INFO, "send status[%ld] to biz %ld, description:%s.", status, id, description.data());
#endif
}

void avs_player_factory::sendDataMsg(long id, long type, long cur, long total) {
    if (m_pMsgCbThread) {
        unifyMsgStruct msg;
        msg.msgType = 1;
        msg.bizId = id;
        msg.bizType = type;
        msg.current = cur;
        msg.total = total;
        m_pMsgCbThread->sendMessage(msg);
    }
}

void avs_player_factory::ff_log_callback_null(void *ptr, int level, const char *fmt, va_list vl) {
    return;
}

void avs_player_factory::mk_log_callback_function(const char *szFileName, int32_t lLine, int32_t lLevel,
                                             const char *format, va_list argp) {
    AS_LOG_WRITE(szFileName, lLine, lLevel, format, argp);
    return;
}
