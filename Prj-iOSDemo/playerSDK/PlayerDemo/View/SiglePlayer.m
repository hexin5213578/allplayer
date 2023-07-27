//
//  SiglePlayer.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/18.
//

#import "SiglePlayer.h"
#import "AllcamApi.h"
#import "AllCamPlayer.h"
#import "openGL20.h"
#import "TimeMarkModel.h"
#import "TimeTool.h"
@interface SiglePlayer()

@property (nonatomic ,strong) OpenGLView20 *playerView;

@property (nonatomic, strong) AllCamPlayer* player;

@property (nonatomic, strong) NSDictionary* playDic;

@property (nonatomic, assign) CGFloat viewWidth;

@property (nonatomic,assign) AllPlayerState state;

@property (nonatomic, strong) NSString* defaultUrl;

@end

@implementation SiglePlayer
- (instancetype)init {
    self = [super init];
    if (self) {
        self.player = [[AllCamPlayer alloc] init];
        self.defaultUrl = @"rtsp://admin:Allcam2019@172.16.20.94/ch1/sub/av_stream";
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PlayerStatueChange:) name:kAllPlayerSdkPlayerStatueChangeNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PlayerDataChange:) name:kAllPlayerSdkPlayerDataNotification object:nil];
    }
    return self;
}


-(void)PlayerStatueChange:(NSNotification*)noti{
    NSDictionary* playerDic = noti.userInfo;
    NSString* status = [playerDic objectForKey:@"status"];
    AllPlayerState playerStatus = (AllPlayerState)[status integerValue];
    NSString* curBid =  [self.player getiCurtBusinessID];

    NSString* lBusiness = [playerDic objectForKey:@"lBusiness"];
    NSLog(@"\n播放状态：%@-%@-%ld",curBid,lBusiness,(long)playerStatus);
    if (![curBid isEqualToString:lBusiness]) {
        return;
    }
    self.state = playerStatus;
    switch (playerStatus) {
    
        case AllPlayerState_Connected:{
            NSString* info = [playerDic objectForKey:@"info"];
            
        }
        break;
        case AllPlayerState_Keyframe:
        case AllPlayerState_Resume:
            {
                
            }
            break;
        case AllPlayerState_Pause:
        case AllPlayerState_Teardown:
        case AllPlayerState_Stop:
            {
               
            }
            break;
        case AllPlayerState_File_Fragment:
            {
                NSString* fileName = [playerDic objectForKey:@"info"];
                [self saveRecordVideo:fileName];
            }
            break;

        case AllPlayerState_THUMBNAIL_SUCCESS:
            {
                NSString* fileName = [playerDic objectForKey:@"info"];
            }
            break;

        case AllPlayerState_TALK_SUCCESS:
            {
                NSLog(@"111111111111111111111111语音对讲开启成功");
            }
            break;
        case AllPlayerState_TALK_FAIL:
            {
                NSLog(@"111111111111111111111111语音对讲开启失败");
            }
            break;
        case 20012:
            {
               NSDictionary *paramsDict =[self.player mediaInfoData];
                NSLog(@"%@",paramsDict);
            }
            break;
        case 10017:
            {
                //NSLog(@"1111");
            }
            break;
        default:
            break;
    }
}
-(void)saveRecordVideo:(NSString*)videoPath{

    //获取缩略图
    NSString* fileName = [[videoPath lastPathComponent] stringByDeletingPathExtension];
    NSString *imgFileName = [fileName stringByAppendingString:@".jpg"];
    NSString* imgFilePath =  [[videoPath stringByDeletingLastPathComponent] stringByAppendingPathComponent:imgFileName];

    //[self.player getThumbnailUrl:imgFilePath withVideoPath:videoPath];
}
-(void)PlayerDataChange:(NSNotification*)noti{
    NSDictionary* playerDic = noti.userInfo;
    NSString* status = [playerDic objectForKey:@"status"];
    NSString* curBid =  [self.player getiCurtBusinessID];
    AllPlayerState playerStatus = (AllPlayerState)[status integerValue];
    NSString* lBusiness = [playerDic objectForKey:@"lBusiness"];
   
    if (![curBid isEqualToString:lBusiness]) {
        return;
    }
    
    if (playerStatus == 20018) {
        NSLog(@"语音对讲开启失败");
    }
    
    if (playerStatus == 20019) {
        NSLog(@"语音对讲开启成功");
    }
}

-(OpenGLView20*)getPlayerView{
    return self.playerView;
}
-(void)playerData:(NSDictionary*)playDic width:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done{
    self.playDic = playDic;
    self.viewWidth = width;
    
    NSString *imagePath = [[NSBundle mainBundle] pathForResource:@"allplayer" ofType:@"ttf"];
    NSDictionary *waterDict = @{
        @"BusinessType":@"6005",
        @"Text":@"全国",
        @"FontColor":@"#000000",
        @"FontFile":imagePath,
        @"FontSize":@30,
        @"Alpha":@0.750000,
        @"Position":@1,                //水印位置，见下
        @"LocalTime":@1,            //水印是否含有本地时间 0-不含有，默认  1-含有
        @"RenderOn":@1                //渲染时水印是否开启， 0-关闭，默认， 1-开启
    };
    [self.player waterParams:[self gs_jsonStringCompactFormatForDictionary:waterDict]];
    
    __weak typeof(self) weakself = self;
    [self getPlayUrlwithDeviceId: [playDic objectForKey:@"deviceId"] done:^(NSString *url) {
        weakself.playerView = [weakself.player getPlayerViewWithUrl:url AndWidth:width];
        //[weakself.player setVolume:100];
        [weakself realPlay];
        done(weakself.playerView,YES);
    }];
}
-(void)getPlayUrlwithDeviceId:(NSString*)deviceId done:(void (^)( NSString *url))done{
    [AllcamApi getMediaLiveUrlWithCameraId:deviceId streamType:@"1" urlType:@"" agentType:@"" Success:^(NSDictionary * _Nonnull result) {
        done(result[@"url"]);
        } failure:^(NSDictionary * _Nonnull error) {
            done(@"");
        }];
}

-(void)defaultPlaywidth:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done{
    self.playerView = [self.player getPlayerViewWithUrl:self.defaultUrl AndWidth:width];;
    done(self.playerView,YES);
}

-(void)loacalPlay:(NSString*)path  width:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done{
    self.playerView = [self.player getPlayerViewWithUrl:path AndWidth:width];
    done(self.playerView,YES);
}
-(void)realPlay{
    [self videoSetting:1 CacheFrame:400];
    [self.player realPlay];
}
-(void)videoSetting:(NSInteger)videoPrior CacheFrame:(NSInteger)videoCache{
    int32_t  cacheframe = (int32_t)(((CGFloat)videoCache/1000)*25);
    int32_t realQualityPrior = (videoPrior == 0)? 0:1;
    [self.player videoSetting:realQualityPrior CacheFrame:cacheframe];
}
-(void)realResume{
    [self.player realResume];
}
-(void)realStop{
    [self.player realStop];
}

-(void)videoPlayerData:(NSDictionary*)playDic width:(CGFloat)width done:(void (^)(OpenGLView20* playew,NSArray* videoArr, BOOL done))done{
    self.playDic = playDic;
    self.viewWidth = width;
    __weak typeof(self) weakself = self;
    [self getVideoUrlwithDeviceId:[playDic objectForKey:@"deviceId"] done:^(NSArray *videoArr, NSString *url) {
        weakself.playerView = [self.player getPlayerViewWithUrl:url AndWidth:width];;
        done(weakself.playerView,videoArr,YES);
    }];
}
-(void)getVideoUrlwithDeviceId:(NSString*)deviceId done:(void (^)( NSArray* videoArr,NSString *url))done{
    NSMutableArray* cameraList = [[NSMutableArray alloc]init];
    NSMutableDictionary* cameraIdDic = [[NSMutableDictionary alloc]init];
    [cameraIdDic setValue:deviceId forKey:@"cameraId"];
    [cameraList addObject:cameraIdDic];
    NSMutableDictionary* searchInfo = [[NSMutableDictionary alloc]initWithDictionary:[self defaultDuration]];
    [searchInfo setValue:@"PLATFORM" forKey:@"from"];
    NSMutableArray* eventList = [[NSMutableArray alloc]init];
    NSMutableDictionary* eventDic = [[NSMutableDictionary alloc]init];
    [eventDic setValue:@"ALL" forKey:@"event"];
    [eventList addObject:eventDic];
    [searchInfo setValue:eventList forKey:@"eventList"];
    
    self.primaryModel = [TimeTool getPrimaryModelfromsTime:[searchInfo objectForKey:@"beginTime"] toeTime:[searchInfo objectForKey:@"endTime"]];
    __weak typeof(self) weakself = self;
    [AllcamApi getRecordListWithCameraList:cameraList searchInfo:searchInfo pageNum:@"1" Success:^(NSDictionary * _Nonnull result) {
        NSArray* videoArr = result[@"recordList"];
        if(videoArr){
            NSDictionary* videDic = [videoArr firstObject];
            [weakself getRecordUrlWith:videDic done:^(NSString *url) {
                done(videoArr,url);
            }];
        }
       
    } failure:^(NSDictionary * _Nonnull error) {
            
    }];
}
-(void)getRecordUrlWith:(NSDictionary*)videoDic done:(void (^)( NSString *url))done{
    [AllcamApi getRecordUrlWithVodInfo:videoDic streamType:@"" urlType:@"" agentType:@"" Success:^(NSDictionary * _Nonnull result) {
       done(result[@"url"]);
        } failure:^(NSDictionary * _Nonnull error) {
        }];
}
-(void)recordPlay{
    [self.player recordPlayWithPlatType:5];
}
-(void)recordStop{
    [self.player recordStop];
}
- (void)palyAtPoint:(double)start WithSpeed:(double)speed{
    [self.player seekToTime:start seekSpeed:speed];
}

-(NSMutableDictionary*)defaultDuration{
    NSMutableDictionary*timeDic = [[NSMutableDictionary alloc]init];
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"YYYY-MM-dd HH:mm:ss"];
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDate *now = [NSDate date];
    NSDateComponents *components = [calendar components:NSCalendarUnitYear|NSCalendarUnitMonth|NSCalendarUnitDay fromDate:now];
    NSDate *startDate = [calendar dateFromComponents:components];
    NSDate *beinDate = [calendar dateByAddingUnit:NSCalendarUnitDay value:0 toDate:startDate options:0];
    NSString *beginTime = [dateFormatter stringFromDate:beinDate];
    [timeDic setValue:beginTime forKey:@"beginTime"];
    NSDate *nextDate = [calendar dateByAddingUnit:NSCalendarUnitDay value:1 toDate:beinDate options:0];
    NSDate *endDate = [calendar dateByAddingUnit:NSCalendarUnitSecond value:-1 toDate:nextDate options:0];
    NSString *endTime = [dateFormatter stringFromDate:endDate];
    [timeDic setValue:endTime forKey:@"endTime"];
    return timeDic;
}


-(void)voiceSetting:(int)value{
    [self.player setVolume:value];
}
-(void)snapShot{
    NSString* fileName = @"";
    if (self.playDic) {
        fileName = [self.playDic objectForKey:@"deviceName"];
    }else{
        fileName = @"默认";
    }
  
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:[fileName stringByAppendingString:@".jpg"]];
    [self.player snapShot:outputFileName];
}
-(void)videoRecord{
    NSString* fileName = @"";
    if (self.playDic) {
        fileName = [self.playDic objectForKey:@"deviceName"];
    }else{
        fileName = @"默认";
    }
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:fileName];
    [self.player saveVideoRecordAt:outputFileName];
}
//-(void)saveRecordVideo:(NSString*)videoPath{
//    NSString* fileName = [[videoPath lastPathComponent] stringByDeletingPathExtension];
//    NSString *floder = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
//    NSString *imgFileName = [fileName stringByAppendingString:@".jpg"];
//    NSString* thumbnailPath = [floder stringByAppendingPathComponent:imgFileName];
//    [self.player getThumbnailUrl:thumbnailPath withVideoPath:videoPath];
//}
-(void)saveRecordThumbnail:(NSString*)thumbnailPath{
    
}
-(void)stopVideoRexord{
    [self.player recordLocalStop];
}
-(void)loacalPlay{
    [self.player localPlayWithPath];
}
-(void)localStop{
    [self.player localStop];
}

- (NSString *)gs_jsonStringCompactFormatForDictionary:(NSDictionary *)dicJson {

    

    if (![dicJson isKindOfClass:[NSDictionary class]] || ![NSJSONSerialization isValidJSONObject:dicJson]) {

        return nil;

    }

    

    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dicJson options:0 error:nil];

    NSString *strJson = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];

    return strJson;

}
@end
