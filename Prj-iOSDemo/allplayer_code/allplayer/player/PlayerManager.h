//
//  PlayerManager.h
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/23.
//

#import <Foundation/Foundation.h>
#import "PJGLKView.h"
#include "avs_egress_decode_ios.hpp"
#include "avs_render_ios.hpp"
#include "avs_render.h"
#include "avs_player.h"
#include "OpenGLView20.h"
NS_ASSUME_NONNULL_BEGIN

@interface PlayerManager : NSObject
{
    avs_egress_decode_ios* m_avsdecode;
    avs_render_ios* m_iosrender;
    avs_render m_avsrender ;
    avs_player* pavc_player;
    
    uint8_t* s_audio_buf;
    uint8_t* s_audio_end;
    uint8_t* s_audio_pos;
}
@property(nonatomic,strong) PJGLKView* playerView;
@property(nonatomic,assign) CGFloat playerWidth;
@property(nonatomic,strong) OpenGLView20 * playerView1;
+ (instancetype)sharedPlayerManager;
-(void)preparePlayer:(NSString*)url;
-(OpenGLView20*)intPlayerView:(CGFloat)playerWidth;
@end

NS_ASSUME_NONNULL_END
