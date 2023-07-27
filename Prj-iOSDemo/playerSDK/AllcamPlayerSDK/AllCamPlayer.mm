//
//  AllCamPlayer.m
//  allplayer_ios
//
//  Created by 于维洋 on 2021/8/28.
//

#import  "AllCamPlayer.h"
#include "allplayer.h"
#include <stdio.h>

#define INVALID_WND -1
long g_iBusinessID = 1;

void test_status_callback(long lBusiness, long busType, long status, const char* info)
{
    NSString *lBusinessString = [NSNumber numberWithLong:lBusiness].description;
    NSString *busTypeString = [NSNumber numberWithLong:busType].description;
    NSString *statusString = [NSNumber numberWithLong:status].description;
    NSString *infostr = [[NSString alloc] initWithUTF8String:info]? [[NSString alloc] initWithUTF8String:info] : @"";
    NSDictionary * dic = @{@"lBusiness":lBusinessString,@"busType":busTypeString,@"status":statusString,@"info":infostr};
    [AllCamPlayer NoticefyChangePlayerStatus:dic];
}
void allplayer_data_callback(long lBusiness, long busType, long current, long total)
{
    NSString *lBusinessString = [NSNumber numberWithLong:lBusiness].description;
    NSString *busTypeString = [NSNumber numberWithLong:busType].description;
    NSString *currentString = [NSNumber numberWithLong:current].description;
    NSString *totalString = [NSNumber numberWithLong:total].description;
    NSDictionary * dic = @{@"lBusiness":lBusinessString,@"busType":busTypeString,@"current":currentString,@"total":totalString };
    [AllCamPlayer NoticefyChangePlayerData:dic];
}

@interface AllCamPlayer()
{
    long m_iCurtBusinessID;
    void *WindowsHandle;
    char Pdata[2048];
}

@property (nonatomic, copy) NSString *rtspUrl;
@property (nonatomic, copy) NSString *realQualityPrior; //默认实时性优先
@property (nonatomic, copy) NSString *cacheSize;
@property (nonatomic, assign) CGFloat playerWidth;
@end

@implementation AllCamPlayer

+(void)NoticefyChangePlayerStatus:(NSDictionary*)statusInfo{
    [[NSNotificationCenter defaultCenter] postNotificationName:kAllPlayerSdkPlayerStatueChangeNotification object:nil userInfo:statusInfo];
}
+(void)NoticefyChangePlayerData:(NSDictionary*)statusInfo{
   
    [[NSNotificationCenter defaultCenter] postNotificationName:kAllPlayerSdkPlayerDataNotification object:nil userInfo:statusInfo];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.realQualityPrior = @"0";
        self.cacheSize = @"";
        ap_lib_reg_data_callback(allplayer_data_callback);
        ap_lib_reg_status_callback(test_status_callback);
    }
    return self;
}

+ (void)initSDK{
    int32_t leaval = 7;
    NSString*documentPath =NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    NSString *outputFileName = [[documentPath stringByAppendingString:@"/"] stringByAppendingString:@"play.log"];
    UInt8 buff_str[1024];
    memcpy(buff_str,[outputFileName UTF8String], [outputFileName length]+1);
    ap_lib_init((char*)buff_str, leaval);
}

+ (NSString*)version{
    return @"5.1.1.1";
}

- (OpenGLView20*)getPlayerViewWithUrl:(NSString*)url AndWidth:(CGFloat)playerWidth{
    self.playerWidth = playerWidth;

    self.rtspUrl = url;
    WindowsHandle =(__bridge void*)self.playerView;
    m_iCurtBusinessID = g_iBusinessID++;
    
    return [self playerView];
}

- (OpenGLView20*)playerView{
    if (!_playerView) {
        _playerView = [[OpenGLView20 alloc] initWithFrame:CGRectMake(0, 44, self.playerWidth, self.playerWidth/16*9)];
    }
    return _playerView;
}

- (void)videoSetting:(int32_t)realQualityPrior CacheFrame:(int32_t)cacheframe{
    self.realQualityPrior = [NSString stringWithFormat:@"%d",realQualityPrior];
    self.cacheSize = [NSString stringWithFormat:@"%d",cacheframe];
}

- (NSDictionary *)mediaInfoData {
    NSDictionary *mediaDict = @{
        @"BusinessType":@"4009"
    };
    
    char tempChar[2048];
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:mediaDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,tempChar);
    
    NSString *mediaData = [[NSString alloc] initWithUTF8String:tempChar];
    NSDictionary *paramsDict = [self toArrayOrNSDictionary:mediaData];
    return paramsDict;
}

- (NSArray *)experienceInfoDataWithBussinessIdArr:(NSArray *)bussinessIdArr {
    NSDictionary *experienceDict = @{
        @"BusinessType":@"6001",
        @"BussinessIdArr":bussinessIdArr
    };
    
    NSLog(@"%@",experienceDict);
    char tempChar[2048];
    
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:experienceDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,tempChar);
    
    NSString *mediaData = [[NSString alloc] initWithUTF8String:tempChar];
    NSArray *paramsArray = [self toArrayOrNSDictionary:mediaData];
    return paramsArray;
}

-(void)audioStartWithRtspUrl:(NSString *)url{
    NSDictionary *audioTalkDict = @{
        @"BusinessType":@"5001",
        @"RtspUrl":url
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:audioTalkDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)audioStop{
    NSDictionary *audioTalkDict = @{
        @"BusinessType":@"5002"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:audioTalkDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)realPlay{
    NSDictionary *realDict = @{
        @"BusinessType":@"1001",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"CacheSize":self.cacheSize,
        @"RealOrQuality":self.realQualityPrior,
        @"RtspUrl":self.rtspUrl
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}


-(void)realStop{
    NSDictionary *realDict = @{
        @"BusinessType":@"1002",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle]
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
    [_playerView removeFromSuperview];
    _playerView = nil;
}

-(void)realPause{
    NSDictionary *realDict = @{
        @"BusinessType":@"1003"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}


-(void)realResume{
    NSDictionary *realDict = @{
        @"BusinessType":@"1004"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}


-(void)recordPlayWithPlatType:(NSInteger)platType{
    NSDictionary *realDict = @{
        @"BusinessType":@"2001",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"CacheSize":self.cacheSize,
        @"RealOrQuality":self.realQualityPrior,
        @"RtspUrl":self.rtspUrl,
        @"ScaleOrSpeed":platType==9?@"1":@"0"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)recordStop{
    NSDictionary *realDict = @{
        @"BusinessType":@"2002",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle]
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
    [_playerView removeFromSuperview];
    _playerView = nil;
}

-(void)recordPause{
    NSDictionary *realDict = @{
        @"BusinessType":@"2003"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}


-(void)recordResume{
    NSDictionary *realDict = @{
        @"BusinessType":@"2004"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)setVolume:(int)volume{
    NSDictionary *realDict = @{
        @"BusinessType":@"4007",
        @"VolumeValue":[NSString stringWithFormat:@"%d",volume]
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)seekToTime:(double)time seekSpeed:(double)speed {
    NSDictionary *realDict = @{
        @"BusinessType":@"2005",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"NptPos":[NSString stringWithFormat:@"%f",time],
        @"PlayScale":[NSString stringWithFormat:@"%f",speed]
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)seekToTime:(double)time seekSpeed:(double)speed platType:(NSInteger)platType{
    NSMutableDictionary *audioTalkDict = [NSMutableDictionary new];
    [audioTalkDict setValue:@"2005" forKey:@"BusinessType"];

    if (platType == 9) {
        if (speed > 0) {
            [audioTalkDict setValue:@"1" forKey:@"ScaleOrSpeed"];
        }
    }

    if (platType == 5) {
        if (speed == -1) {
            [audioTalkDict setValue:@"252" forKey:@"PlayScale"];
        }
        
        if (speed == -2) {
            [audioTalkDict setValue:@"253" forKey:@"PlayScale"];
        }
        
        if (speed == -4) {
            [audioTalkDict setValue:@"-4" forKey:@"PlayScale"];
        }
    }

    if (![[audioTalkDict allKeys] containsObject:@"ScaleOrSpeed"]) {
        [audioTalkDict setValue:@"0" forKey:@"ScaleOrSpeed"];
    }

    if (![[audioTalkDict allKeys] containsObject:@"PlayScale"]) {
        [audioTalkDict setValue:[NSString stringWithFormat:@"%f",speed] forKey:@"PlayScale"];
    }

    [audioTalkDict setValue:[NSString stringWithFormat:@"%f",time] forKey:@"NptPos"];

    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:audioTalkDict.copy] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

- (void)newSeekToTime:(NSString *)params{
    char *cString = (char*)[params cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(NSString *)getiCurtBusinessID{
    NSString *iBusinessString = [NSNumber numberWithLong:m_iCurtBusinessID].description;
    return iBusinessString;
}


-(void)snapShot:(NSString *)path{
    NSDictionary *realDict = @{
        @"BusinessType":@"4006",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"CapturePath":path,
        @"CaptureCount":@"1"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}


-(int32_t)saveVideoRecordAt:(NSString *)path{
    NSDictionary *realDict = @{
        @"BusinessType":@"4004",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"DownloadCutFormat":@"1",
        @"DownloadCutSize":@"1024",
        @"FileFormat":@"0",
        @"RecordDownloadPath":path
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    return ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)recordLocalStop{
    NSDictionary *realDict = @{
        @"BusinessType":@"4005"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

//- (void)getThumbnailUrl:(NSString *)imgPath withVideoPath:(NSString *)videoPath{
//    m_busInfo.Url = [videoPath UTF8String];
//    m_busInfo.BizType = TYPE_URL_THUMBNAIL;
//    m_busInfo.SnapPath = [imgPath UTF8String];
//    [self.player playerExcute:m_iCurtBusinessID + 1000 Business:m_busInfo pResultData:Pdata];
//}
//
//
-(void)localPlayWithPath{
    NSDictionary *realDict = @{
        @"BusinessType":@"7001",
        @"BusinessHwnd":[NSString stringWithFormat:@"%lu", (uintptr_t)WindowsHandle],
        @"Url":self.rtspUrl,
        @"VolumeValue":@"70"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

-(void)localStop{
    NSDictionary *realDict = @{
        @"BusinessType":@"7002"
    };
    char *cString = (char*)[[self gs_jsonStringCompactFormatForDictionary:realDict] cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
    [_playerView removeFromSuperview];
    _playerView = nil;
}

//-(void)localPause{
//    m_busInfo.BizType = TYPE_URL_PAUSE;
//    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo pResultData:Pdata];
//}
//
//-(void)localResume{
//    m_busInfo.BizType = TYPE_URL_RESUE;
//    [self.player playerExcute:m_iCurtBusinessID Business:m_busInfo pResultData:Pdata];
//}

- (void)waterParams:(NSString *)params {
    char *cString = (char*)[params cStringUsingEncoding:NSUTF8StringEncoding];
    ap_lib_excute(m_iCurtBusinessID, cString,Pdata);
}

- (NSString *)gs_jsonStringCompactFormatForDictionary:(NSDictionary *)dicJson {

    if (![dicJson isKindOfClass:[NSDictionary class]] || ![NSJSONSerialization isValidJSONObject:dicJson]) {

        return nil;
    }

    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dicJson options:0 error:nil];

    NSString *strJson = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];

    return strJson;
}

- (id)toArrayOrNSDictionary:(NSString *)jsonData{
    if (jsonData != nil) {
        NSData* data = [jsonData dataUsingEncoding:NSUTF8StringEncoding];
        id jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                        options:NSJSONReadingAllowFragments
                                                          error:nil];
 
        if (jsonObject != nil){
            return jsonObject;
        }else{
            // 解析错误
            return nil;
        }
    }
    return nil;
}
@end
