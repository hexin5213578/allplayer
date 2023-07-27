#pragma once
#include "utils/util.h"

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

#include <vector>
#include <list>
#include <mutex>

struct ACAudioSpec 
{
    int freq = 44100; //采样率
    unsigned short format = AUDIO_S16SYS;
    unsigned char channels = 2;
    unsigned short samples = 1024;
};

struct ACAudioData 
{
    std::vector<unsigned char> data;
    int offset = 0; //偏移位置
    long long pts = 0;
};

class ACAudioPlay
{
public:
    static ACAudioPlay* GetInstance();

    //暂停
    virtual void pause(bool isPause) = 0;

    virtual void push(AVFrame* frame);

    bool initG711Para();

    bool initAudioPara(AVFrame* frame);

    //打开音频 开始播放 调用回调函数
    virtual bool open(ACAudioSpec& spec) = 0;

    virtual void close() = 0;
    
    virtual void clear() {
        close();
        setSpeed(m_speed);
    }

    void push(const unsigned char* data, int size, long long pts);

    //播放速度
    virtual void setSpeed(float speed);

    void setVolume(int volum);

protected:
    ACAudioPlay();

    virtual void Callback(unsigned char* stream, int len) = 0;

    static void AudioCallback(void* userdata, unsigned char* stream, int len) 
    {
        auto ap = (ACAudioPlay*)userdata;
        ap->Callback(stream, len);
    }

protected:
    float m_speed = 1.;

    std::list<ACAudioData>  m_audioDatas;//音频缓冲列表
    std::mutex              m_Mtx;
    // 0~128 音量
    unsigned char           m_volume = 128;
    ACAudioSpec             m_spec;

    bool                    m_inited = false;
};








