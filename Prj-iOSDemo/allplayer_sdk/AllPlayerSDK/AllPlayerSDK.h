//
//  AllPlayerSDK.h
//  AllPlayerSDK
//
//  Created by ZC1AE6-4501-B15A on 2021/5/21.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "PlayerView.h"

typedef NS_ENUM(NSInteger, AllPlayerState)
{
    AllPlayerState_None          = -1,
    AllPlayerState_Connected     = 20001,       //媒体连接已建立
    AllPlayerState_Keyframe,                    //视频渲染第一帧
    AllPlayerState_Pause,                       //暂停
    AllPlayerState_Teardown,                    //rtsp结束
    AllPlayerState_Timeout,                     //数据流接收超时
    AllPlayerState_Stop,                        //资源回收完毕
};

static NSString *const kAllPlayerSdkPlayerStatueChangeNotification = @"PlayerStatueChangeNotification";
static NSString *const kAllPlayerSdkPlayerDataNotification = @"kAllPlayerSdkPlayerDataNotification";

@interface AllPlayerSDK : NSObject

+(void)NoticefyChangePlayerStatus:(NSDictionary*)statusInfo;
+(void)NoticefyChangePlayerData:(NSDictionary*)statusInfo;

-(void)initAllPlayerSDk;

-(PlayerView*)getPlayerViewWithUrl:(NSString*)url AndWidth:(CGFloat)playerWidth;

- (void)realPlay;
- (void)realTimePause;
- (void)realTimeStop;

- (void)recordPlay;
- (void)recordPlayPause;
- (void)recordPlayResume;
- (void)recordPlayStop;

- (void)snapPicture:(NSString*)path;

- (void)palyAtPoint:(double)start WithSpeed:(double)speed;

- (float)getPlaySpeed;

- (NSString*)getiCurtBusinessID;

- (void)setVolume:(CGFloat)volume;

@end
