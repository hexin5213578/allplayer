//
//  avs_render_ios.hpp
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/26.
//

#ifndef avs_render_ios_hpp
#define avs_render_ios_hpp

#include <stdio.h>
#include "avs_render.h"
class avs_render_ios:public avs_render{
    
    avs_render_ios();
    virtual ~avs_render_ios();
public:
//    int32_t init();
//    void release();
//    int32_t snap_picture();
//    void voice_control();
//    int32_t start_egress();
//    int32_t insert_video_data();
//    int32_t insert_audio_data();
    int32_t renderInit();
};
#endif /* avs_render_ios_hpp */
