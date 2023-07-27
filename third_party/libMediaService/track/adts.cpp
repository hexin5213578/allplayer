#include "adts.h"
#include <map>

CADTS::CADTS():m_pBuf(),m_nBit(BYTE_NUMBIT*N_ADTS_SIZE),m_curBit(0)
{

}

void CADTS::init(const SAacParam &aacHead)
{
    /* Fixed ADTS header */
    putBit(0xFFFF, 12);// 12 bit Syncword
    putBit(aacHead.eId, 1); //ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC
    putBit(0, 2); //layer == 0
    putBit(1, 1); //protection absent
    putBit(aacHead.eProfile, 2); //profile
    putBit(CADTS::getSampleIndex(aacHead.nSample), 4); //sampling rate
    putBit(0, 1); //private bit
    putBit(CADTS::getChannelIndex(aacHead.nChannel), 3); //numChannels
    putBit(0, 1); //original/copy
    putBit(0, 1); // home
    /* Variable ADTS header */
    putBit(0, 1); // copyr. id. bit
    putBit(0, 1); // copyr. id. start
    putBit(getBufSize() + aacHead.nPlayod, 13); //ADTS帧的长度包括ADTS头和AAC原始流
    putBit(0x7FF, 11); // buffer fullness (0x7FF for VBR)
    putBit(0 ,2); //raw data blocks (0+1=1)
}

uint8_t *CADTS::getBuf()
{
    return m_pBuf;
}

uint32_t CADTS::getBufSize() const
{
    return m_nBit/BYTE_NUMBIT;
}

int CADTS::putBit(uint32_t data, int numBit)
{
    int num,maxNum,curNum;
    unsigned long bits;

    if (numBit == 0)
        return 0;

    /* write bits in packets according to buffer byte boundaries */
    num = 0;
    maxNum = BYTE_NUMBIT - m_curBit % BYTE_NUMBIT;
    while (num < numBit) {
        curNum = std::min(numBit-num,maxNum);
        bits = data>>(numBit-num-curNum);
        if (writeByte(bits, curNum)) {
            return 1;
        }
        num += curNum;
        maxNum = BYTE_NUMBIT;
    }

    return 0;
}

int CADTS::writeByte(uint32_t data, int numBit)
{
    long numUsed,idx;

    idx = (m_curBit / BYTE_NUMBIT) % N_ADTS_SIZE;
    numUsed = m_curBit % BYTE_NUMBIT;
#ifndef DRM
    if (numUsed == 0)
        m_pBuf[idx] = 0;
#endif
    m_pBuf[idx] |= (data & ((1<<numBit)-1)) << (BYTE_NUMBIT-numUsed-numBit);
    m_curBit += numBit;

    return 0;
}


eAACSample CADTS::getSampleIndex(const uint32_t nSample)
{
    eAACSample eSample = E_AAC_SAMPLE_RESERVED;
    static std::map<uint32_t, eAACSample> mpSample;
    if (mpSample.empty())
    {
        mpSample[96000] = E_AAC_SAMPLE_96000_HZ;
        mpSample[88200] = E_AAC_SAMPLE_88200_HZ;
        mpSample[64000] = E_AAC_SAMPLE_64000_HZ;
        mpSample[48000] = E_AAC_SAMPLE_48000_HZ;
        mpSample[44100] = E_AAC_SAMPLE_44100_HZ;
        mpSample[32000] = E_AAC_SAMPLE_32000_HZ;
        mpSample[24000] = E_AAC_SAMPLE_24000_HZ;
        mpSample[22050] = E_AAC_SAMPLE_22050_HZ;
        mpSample[16000] = E_AAC_SAMPLE_16000_HZ;
        mpSample[12000] = E_AAC_SAMPLE_12000_HZ;
        mpSample[11025] = E_AAC_SAMPLE_11025_HZ;
        mpSample[8000]  = E_AAC_SAMPLE_8000_HZ;
        mpSample[7350]  = E_AAC_SAMPLE_7350_HZ;
    };
    if (mpSample.find(nSample) != mpSample.end())
    {
        eSample = mpSample[nSample];
    }
    return eSample;
}

eAACChannel CADTS::getChannelIndex(const uint32_t nChannel)
{
    eAACChannel eChannel = E_AAC_CHANNEL_RESERVED;
    static std::map<uint32_t, eAACChannel> mpChannel;
    if (mpChannel.empty())
    {
        mpChannel[0] = E_AAC_CHANNEL_SPECIFC_CONFIG;
        mpChannel[1] = E_AAC_CHANNEL_MONO;
        mpChannel[2] = E_AAC_CHANNEL_STEREO;
        mpChannel[3] = E_AAC_CHANNEL_TRIPLE_TRACK;
        mpChannel[4] = E_AAC_CHANNEL_4;
        mpChannel[5] = E_AAC_CHANNEL_5;
        mpChannel[6] = E_AAC_CHANNEL_6;
        mpChannel[8] = E_AAC_CHANNEL_8;
    };
    if (mpChannel.find(nChannel) != mpChannel.end())
    {
        eChannel = mpChannel[nChannel];
    }
    return eChannel;
}

