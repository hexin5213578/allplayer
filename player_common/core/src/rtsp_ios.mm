//
//  rtsp_ios.cpp
//  playerSDK
//
//  Created by yuweiyang on 2021/9/6.
//

#include "rtsp_ios.hpp"
#import "ios_openGL_view.h"
#include "openAL_player.h"
extern "C"
{
#include <libavcodec/avcodec.h>
}


//IOSRtspPlayer::IOSRtspPlayer(BizParams& bizParam): RtspPlayer(bizParam)
//{
//    [[OpenalPlayer sharedInstance]  initOpenAL];
//}
//
//void IOSRtspPlayer::pause(bool isPause, bool needClear)
//{
//    RtspPlayer::pause(isPause, needClear);
//    if(isPause) {
//        [[OpenalPlayer sharedInstance] pauseSound];
//    }
//    else {
//       [[OpenalPlayer sharedInstance] playSound];
//    }
//}

//void IOSRtspPlayer::disPlayAudio(AVFrame* frame)
//{
//    if (!frame) {
//        return;
//    }
//
//    if (m_bSoundPlay) {
//        uint8_t* tempData;
//        tempData = frame->data[0];
//        int linesize = frame->linesize[0];
//        [[OpenalPlayer sharedInstance]openAudioFromQueue:(char*)tempData andWithDataSize:linesize andWithSampleRate:8000 andWithAbit:16 andWithAchannel:1];
//    }
//}

//void IOSRtspPlayer::setVolume(int16_t volume)
//{
//   RtspPlayer::setVolume(volume);
//   [[OpenalPlayer sharedInstance] setM_volume:volume];
//}
//
//
//IOSRtspPlayer::~IOSRtspPlayer()
//{
//}
