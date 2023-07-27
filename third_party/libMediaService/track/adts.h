#pragma once
#include <cstdint>

#define BYTE_NUMBIT 8       /* bits in byte (char) */

#define N_ADTS_SIZE 7
/*
 * 定义是哪个级别的AAC
 */
enum eAACProfile
{
    E_AAC_PROFILE_MAIN_PROFILE = 0,
    E_AAC_PROFILE_LC,
    E_AAC_PROFILE_SSR,
    E_AAC_PROFILE_PROFILE_RESERVED,
};

enum eAACSample
{
    E_AAC_SAMPLE_96000_HZ = 0,
    E_AAC_SAMPLE_88200_HZ,
    E_AAC_SAMPLE_64000_HZ,
    E_AAC_SAMPLE_48000_HZ,
    E_AAC_SAMPLE_44100_HZ,
    E_AAC_SAMPLE_32000_HZ,
    E_AAC_SAMPLE_24000_HZ,
    E_AAC_SAMPLE_22050_HZ,
    E_AAC_SAMPLE_16000_HZ,
    E_AAC_SAMPLE_12000_HZ,
    E_AAC_SAMPLE_11025_HZ,
    E_AAC_SAMPLE_8000_HZ,
    E_AAC_SAMPLE_7350_HZ,
    E_AAC_SAMPLE_RESERVED,
};

enum eAACChannel
{
    E_AAC_CHANNEL_SPECIFC_CONFIG = 0,
    E_AAC_CHANNEL_MONO,
    E_AAC_CHANNEL_STEREO,
    E_AAC_CHANNEL_TRIPLE_TRACK,
    E_AAC_CHANNEL_4,
    E_AAC_CHANNEL_5,
    E_AAC_CHANNEL_6,
    E_AAC_CHANNEL_8,
    E_AAC_CHANNEL_RESERVED,
};

enum eMpegId
{
    E_MPEG4 = 0,
    E_MPEG_2
};


struct SAacParam
{
    SAacParam(uint32_t playod, int32_t sample, int32_t channel = 1, eAACProfile profile = E_AAC_PROFILE_LC, eMpegId id = E_MPEG4)
            :eId(id), eProfile(profile), nChannel(channel), nSample(sample), nPlayod(playod)
    {

    };
    eMpegId eId;
    eAACProfile eProfile;
    int32_t nChannel;
    int32_t nSample;
    uint32_t nPlayod;   //aac负载大小(不包含ADTS头)
};

class CADTS
{
public:
    CADTS();

public:
    /*
     * 初始化函数完成ADTS头的填充
     */
    void init(const SAacParam& aacHead);

    /*
     * 获取ADTS头地址
     */
    uint8_t* getBuf();

    /*
     * 获取ADTS头长度(字节)
     */
    uint32_t getBufSize() const ;

private:
    int putBit(uint32_t data, int numBit);

    int writeByte(uint32_t data, int numBit);
    /*
     * 采样率下标
     */
    static eAACSample getSampleIndex(const uint32_t nSample);

    /*
     * 声道下标
     */
    static eAACChannel getChannelIndex(const uint32_t nChannel);

private:
    uint8_t                 m_pBuf[N_ADTS_SIZE]; //buffer的头指针
    const uint32_t          m_nBit;  //总位数
    uint32_t                m_curBit; //当前位数
};
