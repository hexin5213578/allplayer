//
// Created by HP1 on 2022/3/17.
//

#pragma once
#include <mutex>
#include <queue>

class AudioCollector {
public:
    static AudioCollector* GetInstance();

    struct audioFrame {
        unsigned char *data;
        unsigned int dataSize;
        audioFrame() {
            data = nullptr;
            dataSize = 0;
        }
    };

    void pushAudioFrame(unsigned char* pData, unsigned int ulLen);

    int32_t popAudioFrame(unsigned char* pData, unsigned int* ulLen);

    void stopRecord();

protected:
    AudioCollector();
    virtual  ~AudioCollector();

private:
    static std::mutex       m_mMtx;
    static AudioCollector*  m_pAudioCollector;

    std::mutex          m_mutex;
    char*				m_pAudioData;	//音频数据内存
    unsigned long		m_ulAudioLen;	//音频数据长度
};
