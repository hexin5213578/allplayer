//
//  IOS_AudioCollector.h
//  playerSDK
//
//  Created by yuweiyang on 2022/3/18.
//

#pragma once
#include <mutex>
#ifdef __OBJC__
@class Recorder;
#else
typedef struct objc_object Recorder;
#endif

class IOS_AudioCollector
{
public:
    static IOS_AudioCollector* GetInstance();
    
    void startRecord();
    
    void stopRecord();
    
    int getAudioFrame(unsigned char* data, unsigned int* dataSize);
    
private:
    IOS_AudioCollector();
    virtual ~IOS_AudioCollector();
    static IOS_AudioCollector* audioCollector;
    static std::mutex m_singleMtx;
    
    Recorder* m_auidoRecorder;
};
