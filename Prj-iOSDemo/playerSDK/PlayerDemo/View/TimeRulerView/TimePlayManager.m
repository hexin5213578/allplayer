//
//  TimePlayManager.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimePlayManager.h"
#import "TimeTool.h"
#import "AllcamApi.h"
@implementation TimePlayManager

-(void)getVideoArrWithCameraId:(NSString*)cameraId StartTime:(NSString*)startTime EndTime:(NSString*)endTime withForm:(NSString*)form finishBlock:(void(^)(NSMutableArray* videoArr))finishBlock{
    NSMutableArray* cameralist = [[NSMutableArray alloc]init];
    NSMutableDictionary* cameraIdDic = [[NSMutableDictionary alloc]init];
    [cameraIdDic setValue:cameraId forKey:@"cameraId"];
    [cameralist addObject:cameraIdDic];
    
    NSMutableDictionary* searchInfo = [[NSMutableDictionary alloc]init];
    [searchInfo setValue:startTime forKey:@"beginTime"];
    [searchInfo setValue:endTime forKey:@"endTime"];
    [searchInfo setValue:form forKey:@"from"];
    NSMutableArray* eventlist = [[NSMutableArray alloc]init];
    NSMutableDictionary* eventDic = [[NSMutableDictionary alloc]init];
    [eventDic setValue:@"ALL" forKey:@"event"];
    [eventlist addObject:eventDic];
    [searchInfo setValue:eventlist forKey:@"eventList"];
//自测ok
    [AllcamApi getRecordListWithCameraList:[[NSMutableArray alloc]initWithArray:cameralist] searchInfo:searchInfo pageNum:@"1" Success:^(NSDictionary * _Nonnull result) {
        NSError* error;
    } failure:^(NSDictionary * _Nonnull error) {
        finishBlock(nil);
    }];

}

-(void)getPlayUrlwithRecordInfo:(NSDictionary*)playDic finishBlock:(void(^)(NSString* playUrl))finishBlock
{
    [AllcamApi getRecordUrlWithVodInfo:playDic streamType:@"1" urlType:@"1" agentType:@"" Success:^(NSDictionary * _Nonnull result) {
        finishBlock(result[@"url"]);
        } failure:^(NSDictionary * _Nonnull error) {
            finishBlock(@"");
        }];
}

@end
