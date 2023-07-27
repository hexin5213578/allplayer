//
//  IOS_AudioCollector.m
//  playerSDK
//
//  Created by yuweiyang on 2022/3/18.
//

#include "ios_collector.h"
#import "recorder.h"

std::mutex IOS_AudioCollector::m_singleMtx;
IOS_AudioCollector* IOS_AudioCollector::audioCollector = nullptr;

IOS_AudioCollector* IOS_AudioCollector::GetInstance()
{
    if(audioCollector) {
        return audioCollector;
    }
    
    std::lock_guard<std::mutex> lck(m_singleMtx);
    audioCollector = new IOS_AudioCollector();
    return audioCollector;
}

void IOS_AudioCollector::startRecord()
{
    [m_auidoRecorder start:8000 :1 :16];
}

void IOS_AudioCollector::stopRecord()
{
    [m_auidoRecorder stop];
}

int IOS_AudioCollector::getAudioFrame(unsigned char* data, unsigned int* dataSize)
{
    [m_auidoRecorder getAudioData:data andSize:dataSize];
    return *dataSize;
}

IOS_AudioCollector::IOS_AudioCollector()
{
    m_auidoRecorder= [[Recorder alloc] init];
}

IOS_AudioCollector::~IOS_AudioCollector()
{
    //[m_auidoRecorder dealloc];
    m_auidoRecorder = nullptr;
}


