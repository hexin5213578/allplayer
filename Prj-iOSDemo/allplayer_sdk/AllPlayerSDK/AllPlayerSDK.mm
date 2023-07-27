//
//  AllPlayerSDK.m
//  AllPlayerSDK
//
//  Created by ZC1AE6-4501-B15A on 2021/5/21.
//

#import "AllPlayerSDK.h"
#import "libAllplayer.h"
#include "avs_player.h"
long g_iBusinessID = 1;

void test_status_callback(long lBusiness, long busType, long status, const char* info)
{
    NSString *lBusinessString = [NSNumber numberWithLong:lBusiness].description;
    NSString *busTypeString = [NSNumber numberWithLong:busType].description;
    NSString *statusString = [NSNumber numberWithLong:status].description;
    NSString *infostr = [[NSString alloc] initWithUTF8String:info]? [[NSString alloc] initWithUTF8String:info] : @"";
    NSDictionary * dic = @{@"lBusiness":lBusinessString,@"busType":busTypeString,@"status":statusString,@"info":infostr};
    [AllPlayerSDK NoticefyChangePlayerStatus:dic];
}
void allplayer_data_callback(long lBusiness, long busType, long current, long total)
{
    NSString *lBusinessString = [NSNumber numberWithLong:lBusiness].description;
    NSString *busTypeString = [NSNumber numberWithLong:busType].description;
    NSString *currentString = [NSNumber numberWithLong:current].description;
    NSString *totalString = [NSNumber numberWithLong:total].description;
    NSDictionary * dic = @{@"lBusiness":lBusinessString,@"busType":busTypeString,@"current":currentString,@"total":totalString };
    [AllPlayerSDK NoticefyChangePlayerData:dic];
}
@interface AllPlayerSDK()
{
    long m_iCurtBusinessID;
    BusinessInfoStruct m_busInfo;
}
@property(nonatomic,strong)PlayerView* playerView;
@property(nonatomic,strong) libAllplayer* player;
@property(nonatomic,assign) CGFloat playerWidth;
@end

@implementation AllPlayerSDK
+(void)NoticefyChangePlayerStatus:(NSDictionary*)statusInfo{
    [[NSNotificationCenter defaultCenter] postNotificationName:kAllPlayerSdkPlayerStatueChangeNotification object:nil userInfo:statusInfo];
}
+(void)NoticefyChangePlayerData:(NSDictionary*)statusInfo{
    [[NSNotificationCenter defaultCenter] postNotificationName:kAllPlayerSdkPlayerDataNotification object:nil userInfo:statusInfo];
}
-(void)initAllPlayerSDk
{
    m_iCurtBusinessID = -1;
    int32_t leaval = 7;
    NSString*documentPath =NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    NSString *outputFileName = [[documentPath stringByAppendingString:@"/"] stringByAppendingString:@"play.log"];
    UInt8 buff_str[1024];
    memcpy(buff_str,[outputFileName UTF8String], [outputFileName length]+1);
    self.player = [[libAllplayer alloc]init:(char*)buff_str Loglevel:leaval];
    [self.player  registerPlayerStatusCallback:test_status_callback];
    [self.player  registerPlayerDataCallback:allplayer_data_callback];
}
-(PlayerView*)getPlayerViewWithUrl:(NSString*)playerurl AndWidth:(CGFloat)playerWidth{
    self.playerWidth = playerWidth;
    
    NSString*documentPath =NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    BusinessInfoStruct busInfo;
    busInfo.Url = [playerurl UTF8String];
    busInfo.RecordPath = [documentPath UTF8String];
    
    busInfo.BussinessType = TYPE_REALVIDEO_START;
    
    busInfo.WindowsHandle =(__bridge void*)self.playerView;
    
    m_iCurtBusinessID = g_iBusinessID++;
    
    m_busInfo = busInfo;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
    
    return [self playerView];
}

-(PlayerView*)playerView{
    if (!_playerView) {
        _playerView = [[PlayerView alloc] initWithFrame:CGRectMake(0, 44, self.playerWidth, self.playerWidth/16*9)];
    }
    return _playerView;
}

- (void)realPlay{
    m_busInfo.BussinessType = TYPE_REALVIDEO_START;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}
- (void)realTimePause{
    m_busInfo.BussinessType = TYPE_REALVIDEO_PAUSE;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}
- (void)realTimeStop{
    m_busInfo.BussinessType = TYPE_REALVIDEO_STOP;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}

- (void)recordPlay{
    m_busInfo.BussinessType = TYPE_NETRECORD_START;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}

- (void)recordPlayStop{
    m_busInfo.BussinessType = TYPE_NETRECORD_STOP;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}
- (void)recordPlayPause{
    m_busInfo.BussinessType = TYPE_NETRECORD_PAUSE;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}

- (void)recordPlayResume{
    m_busInfo.BussinessType = TYPE_NETRECORD_RESUME;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}

- (void)snapPicture:(NSString*)path{
    m_busInfo.SnapPath = [path UTF8String];
    m_busInfo.BussinessType = TYPE_REALVIDEO_CAPTURE;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}

- (void)palyAtPoint:(double)start WithSpeed:(double)speed{
    m_busInfo.Scale = speed;
    m_busInfo.Start = start;
    m_busInfo.BussinessType = TYPE_NETRECORD_CONTROL;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}
- (float)getPlaySpeed{
    float  speed = m_busInfo.Scale;
    return speed;
}
- (void)setVolume:(CGFloat)volume{
    m_busInfo.VolumeControl = (int8_t)volume;
    m_busInfo.BussinessType = TYPE_VOLUME_CONTROL;
    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo];
}
- (NSString*)getiCurtBusinessID{
    NSString *lBusinessString = [NSNumber numberWithLong:m_iCurtBusinessID].description;
    return lBusinessString;
}
@end
