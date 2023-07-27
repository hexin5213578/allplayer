//
//  TimePlayManager.h
//  ChinaMobileProject
//  时间播放管理 - 进度播放
//  Created by 秦骏 on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN


@interface TimePlayManager : NSObject
@property (nonatomic, strong) NSMutableDictionary* CarmeraVideDic;
-(void)getVideoArrWithCameraId:(NSString*)cameraId StartTime:(NSString*)startTime EndTime:(NSString*)endTime withForm:(NSString*)form finishBlock:(void(^)(NSMutableArray* videoArr,NSError * error))finishBlock;

-(void)getPlayUrlwithRecordInfo:(NSDictionary*)playDic finishBlock:(void(^)(NSString* playUrl))finishBlock;
@end

NS_ASSUME_NONNULL_END
