#pragma once

#include "avs_video_state.h"

#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB

//int32 support
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB

//float32 support
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#else
#define AUDIO_U16SYS    AUDIO_U16MSB
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_S32SYS    AUDIO_S32MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#endif

const int AUIDO_MAX_SIZE = 40960;

#include <vector>
#include <list>
#include "libMediaService.h"
#include "as_ring_cache.h"

struct AvsAudioData {
    std::vector<unsigned char> data;
    int offset = 0; //偏移位置
};

class AvsAudioPlay
{
public:
    static AvsAudioPlay* GetInstance();

    static AvsAudioPlay* CreateInstance();

    static void DestroyInstance(AvsAudioPlay* play);

    void setVideoState(VideoState* ic) { m_videoState = ic; }

    virtual void pause(bool isPause) = 0;
    virtual void close() = 0;

    virtual void push(AVFrame* frame);
    void push(const unsigned char* data, int size, int64_t pts);

	template <typename T>
    bool initAudioPara(T* params) {
        if (!params) {
            return false;
        }

        if (m_audioParams.channels == params->channels &&
            m_audioParams.freq == params->sample_rate &&
            m_audioParams.fmt == params->format) {
            return true;
        }

        clear();
        m_audioCache.SetCacheSize(8 * params->channels * params->sample_rate);
        auto ret = open(params->channel_layout, params->channels, params->sample_rate);
        return ret >= 0;
    }

    //打开音频 开始播放 调用回调函数
    virtual int open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate) = 0;
    virtual void clear();

    //播放速度
    virtual void setSpeed(float speed);

    void setVolume(int16_t volum);

    as_ring_cache* getPCMCache();

    static uint8_t audioBuffer[AUIDO_MAX_SIZE] ;

protected:
    AvsAudioPlay();

    virtual void callback(unsigned char* stream, int len)  { }

    static void audioCallback(void* userdata, unsigned char* stream, int len) {
        AvsAudioPlay* ap = (AvsAudioPlay*)userdata;
        ap->callback(stream, len);
    }

protected:
    float                   m_speed;
    //音频缓冲数据
    as_ring_cache           m_audioCache; 
    // 0~128 音量
    unsigned char           m_volume;
	struct AudioParams      m_audioParams;
    bool                    m_inited;
    VideoState*             m_videoState;
};








