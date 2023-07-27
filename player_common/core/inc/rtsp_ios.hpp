//
//  rtsp_ios.hpp
//  playerSDK
//
//  Created by yuweiyang on 2021/9/6.
//

#ifndef rtsp_ios_hpp
#define rtsp_ios_hpp

#include "avs_rtsp_player.h"
#include <stdio.h>

class IOSOpenGLVideoView;

class IOSAvsRtspPlayer : public AvsRtspPlayer
{
public:
    IOSAvsRtspPlayer(BizParams& bizParam);
    
    ~IOSAvsRtspPlayer();
    
protected:
    void pause(bool isPause, bool needClear) override;

//    void disPlayImag(AVFrame* frame) override;
//
   // void disPlayAudio(AVFrame* frame) override;

    void setVolume(int16_t volume);
    
private:
    IOSOpenGLVideoView* m_glView = nullptr;
};

#endif /* rtsp_ios_hpp */
