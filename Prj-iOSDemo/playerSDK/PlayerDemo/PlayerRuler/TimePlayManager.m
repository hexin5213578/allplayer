//
//  TimePlayManager.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimePlayManager.h"
#import "GetVideoDataService.h"
#import "TimeTool.h"
#import "GetVideoUrlService.h"

@implementation TimePlayManager

-(void)getVideoArrWithCameraId:(NSString*)cameraId StartTime:(NSString*)startTime EndTime:(NSString*)endTime withForm:(NSString*)form finishBlock:(void(^)(NSMutableArray* videoArr,NSError * error))finishBlock{
    
    NSArray * cameraList = [NSArray arrayWithObject:cameraId];
  
    GetVideoDataService * service = [[GetVideoDataService alloc]initWithfrom:form pageNum:1 pageSize:5000 beginTime:startTime endTime:endTime cameraIdList:cameraList eventList:@[@"ALL"] url:videoListUrl];
    [service startAsynchronous];
    service.getVideoDataBlock = ^(HttpService *service,NSError * error, GetVideoDataRspMsgJM *msgBody) {
        NSLog(@"获取到录像数据");
        if (error) {
            finishBlock(nil,error);
        }else{
            NSMutableArray* videoArr = [[NSMutableArray alloc]init];
            if (msgBody.pageInfo.totalNum != 0) {
                for (RecordList *model in msgBody.recordList) {
                    [videoArr addObject:model];
                }
            }
            finishBlock(videoArr,error);
        }

    };
}

-(void)getPlayUrlwithRecordInfo:(RecordList*)model finishBlock:(void(^)(NSString* playUrl))finishBlock
{
    GetVideoUrlService * service = [[GetVideoUrlService alloc]initWithUrl:getVideoUrl streamType:@"1" urlType:@"1" vodType:@"" recordInfo:model];
    [service startAsynchronous];
    service.getPlayDataBlock = ^(HttpService *service,NSError * error, GetVideoUrlRespJM *msgBody) {
        if (msgBody) {
            NSString* playUrl = msgBody.url;
            finishBlock(playUrl);
        }else{
            finishBlock(@"");
        }

    };
}

@end
