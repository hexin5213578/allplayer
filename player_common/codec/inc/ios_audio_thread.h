//
//  ios_audio_thread.h
//  playerSDK
//
//  Created by sg on 2021/29/12.
//

#ifndef ios_audio_thread_h
#define ios_audio_thread_h

#include "avs_audio_thread.h"


class IOSAudioThread : public AvsAudioThread
{
public:
    IOSAudioThread(VideoState *ic);
    
    int processFrame(AVFrame* frame) override;
};

#endif /* ios_video_thread_h */
