//
// Created by HP1 on 2022/3/17.
//

#include "audio_collector.h"
#include "as_mem.h"

#define  MAX_AUDIO_LEN             10*1024	//10K大小

std::mutex      AudioCollector::m_mMtx;

AudioCollector *AudioCollector::GetInstance()
{
    
    static AudioCollector m_collector;
    return  &m_collector;
}

void AudioCollector::pushAudioFrame(unsigned char *pData, unsigned int ulLen)
{
    std::unique_lock<std::mutex> lck(m_mutex);
    if (m_pAudioData) {
        memcpy(m_pAudioData, pData, ulLen);
        m_ulAudioLen = ulLen;
    }
}

int32_t AudioCollector::popAudioFrame(unsigned char *pData, unsigned int *ulLen)
{
    std::unique_lock<std::mutex> lck(m_mutex);
    if (pData && m_pAudioData && m_ulAudioLen > 0 && m_ulAudioLen < MAX_AUDIO_LEN) {
        memcpy(pData, m_pAudioData, m_ulAudioLen);
        *ulLen = m_ulAudioLen;
        m_ulAudioLen = 0;
    }
    return *ulLen;
}

void AudioCollector::stopRecord()
{

}

AudioCollector::AudioCollector()
{
    AS_NEW(m_pAudioData, MAX_AUDIO_LEN);
}

AudioCollector::~AudioCollector()
{
    AS_DELETE(m_pAudioData, MAX_AUDIO_LEN);
}
