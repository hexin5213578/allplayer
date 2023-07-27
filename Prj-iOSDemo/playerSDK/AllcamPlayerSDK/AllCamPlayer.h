//
//  AllCamPlayer.h
//  allplayer_ios
//
//  Created by 于维洋 on 2021/8/28.
//

#import <Foundation/Foundation.h>
#import "openGL20.h"

NS_ASSUME_NONNULL_BEGIN
/**
 @brief 播放状态
 */
typedef NS_ENUM(NSInteger, AllPlayerState)
{
    AllPlayerState_None                 = -1,
    AllPlayerState_Connected            = 20001,       //媒体连接已建立
    AllPlayerState_Keyframe             = 20002,       //视频渲染第一帧
    AllPlayerState_Pause                = 20003,       //暂停
    AllPlayerState_Resume               = 20004,       //恢复
    AllPlayerState_Teardown             = 20005 ,      //rtsp结束
    AllPlayerState_Timeout              = 20006,       //数据流接收超时
    AllPlayerState_Stop                 = 20007,       //资源回收完毕
    AllPlayerState_Rtcp_TimeOut         = 20008,       //RTCP信令超时
    AllPlayerState_CONN_ERROR           = 20009,       //连接关闭，服务器返回错误等异常情况
    AllPlayerState_SRV_ERROR            = 20010,       //服务端错误
    AllPlayerState_EOS                  = 20011,       //end of stream
    AllPlayerState_SETUP                = 20012,       //RTSP SETUP成功，可PLAY
    AllPlayerState_IO_FINISH            = 20013,       //io结束(写mp4)
    AllPlayerState_STREAM_CONNECT_FAILED= 20014,       //connect失败(超时时间：1.5s)
    AllPlayerStat_PAUSE_RESP            = 20015,       //pause返回200 ok
    AllPlayerState_CONN_CLOSE           = 20016,       	//连接关闭(recv返回0,由服务端主动关闭连接)
    AllPlayerState_FRAGS_MISMATCH       = 20017,       //拼接流多片段数量不匹配
    AllPlayerState_TALK_FAIL            = 20018,       //语音对讲开启失败
    AllPlayerState_TALK_SUCCESS         = 20019,       //对讲开启成功
    AllPlayerState_TALK_SUCCESS_FAIL    = 20020,       //语音对讲失败(开启成功后)
    AllPlayerState_File_Fragment        = 10004,       //录像结束
    AllPlayerState_THUMBNAIL_SUCCESS    = 10009,        //获取缩略图成功 会重生5次
    AllPlayerState_THUMBNAIL_FAILED     = 10010,        //获取缩略图失败
    AllPlayerState_URL_PLAY_EOF         = 10015,        //读取到文件结尾
    AllPlayerState_FILE_WRITE_ERROR     = 10017,        //文件写入失败(无磁盘空间，分辨率变化etc.)
    AllPlayerState_VIDEO_STALLING_EVENT = 50001,        //卡顿异常事件
    AllPlayerState_VIDEO_FREEZING_EVENT = 50002,        //跳帧异常事件
};


/**
 @brief 通知名称
 */
static NSString *const kAllPlayerSdkPlayerStatueChangeNotification = @"PlayerStatueChangeNotification";
static NSString *const kAllPlayerSdkPlayerDataNotification = @"kAllPlayerSdkPlayerDataNotification";
@interface AllCamPlayer : NSObject

+(void)NoticefyChangePlayerStatus:(NSDictionary*)statusInfo;
+(void)NoticefyChangePlayerData:(NSDictionary*)statusInfo;

///**
// @brief 单例
// */
//+ (AllCamPlayer *)share;


/**
 @brief initSDK
 */
+(void)initSDK;

/// 播放器版本
+(NSString*)version;


/**
 @brief 从url获取数据
 */
-(OpenGLView20*)getPlayerViewWithUrl:(NSString*)url AndWidth:(CGFloat)playerWidth;
/**
 @brief 视频设置 realQualityPrior 0 实时优先 1画质优先 cacheframe 缓冲区大小 1s = 25帧
 */
-(void)videoSetting:(int32_t)realQualityPrior CacheFrame:(int32_t)cacheframe;

/**
 @brief 开始实时浏览播放
 */
-(void)realPlay;

/**
 @brief 开始音频采集。
 */
- (void)audioStartWithRtspUrl:(NSString *)url;

/**
 @brief 结束音频采集。
 */
- (void)audioStop;

/**
 @brief 实时浏览暂停
 */
-(void)realStop;

/**
 @brief 实时浏览继续播放
 */
-(void)realResume;

/**
 @brief 实时浏览暂停播放
 */
-(void)realPause;

/**
 @brief 录像开始播放
 */
-(void)recordPlayWithPlatType:(NSInteger)platType;

/**
 @brief 录像暂停播放
 */
-(void)recordPause;

/**
 @brief 录像停止播放
 */
-(void)recordStop;

/**
 @brief 录像继续播放
 */
-(void)recordResume;

/**
 @brief 本地录像结束
 */
-(void)recordLocalStop;


/**
 @brief 跳转到指定的播放位置及倍速播放（仅限录像回放使用）
 @param time 新的播放位置距离初始点的位置（没有拖拽的情况，只是倍速播放的时候time传 -1。拖拽情况下time传结束位置的时间减去初始位置的时间差单位为秒）
 @param speed 速度
 */
-(void)seekToTime:(double)time seekSpeed:(double)speed;

-(void)seekToTime:(double)time seekSpeed:(double)speed platType:(NSInteger)platType;

/**
 @brief 截图
 @param path 图片的路径
 */
-(void) snapShot:(NSString*)path;

/**
 @brief 录像
 @param path 录像的路径
 */
-(int32_t) saveVideoRecordAt:(NSString *)path;

/**
 @brief 本地录像开始播放
 */
-(void)localPlayWithPath;

/**
 @brief 本地录像结束播放
 */
-(void)localStop;

/**
 @brief 本地录像暂停播放
 */
-(void)localPause;

/**
 @brief 本地录像继续播放
 */
-(void)localResume;

/**
 @brief 跳转到指定的播放位置（本地播放）
 @param time 取值0-1
 */
-(void)seekToTime:(double)time;
-(void)newSeekToTime:(NSString *)params;

/**
 @brief 获取当前的镜头。
 */
- (NSString*)getiCurtBusinessID;
/**
 @brief 设置音量。
 */
- (void)setVolume:(int)volume;


/**
 @brief 设置播放器的视图playerView
 *  iOS下为UIView
 */
@property(nonatomic, strong) OpenGLView20* playerView;

///水印
- (void)waterParams:(NSString *)params;

///获取播放器mediaInfo参数信息
- (NSDictionary *)mediaInfoData;

///获取播放器下行流量等参数信息
- (NSArray *)experienceInfoDataWithBussinessIdArr:(NSArray *)bussinessIdArr;
@end

NS_ASSUME_NONNULL_END
