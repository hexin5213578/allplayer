//
//  TimeTool.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/10.
//  Copyright © 2021 hello. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DurationModel.h"
#import "TimeMarkModel.h"
#import "TimeRulerMark.h"
NS_ASSUME_NONNULL_BEGIN

@interface TimeTool : NSObject
//获取今日凌晨到当前时间的秒数
+(CGFloat)getcurrentTimeOffset:(CGFloat)unitWidth;
//获取默认时间段
+(DurationModel*)getCurrentTimeDefaultVideoDuration;
//获取起始终止时刻时间轴初始值
+(TimeMarkModel*)getPrimaryModelfromsTime:(NSString*)startTime toeTime:(NSString*)endTime;
//获取时间间隔
+(CGFloat)getIntervalfTime:(NSString*)cTime since:(NSString*)oTime;
//获得偏移标签
+(NSString*)getMarkStrAtTime:(NSString*)startTime offsetTime:(CGFloat)time TimeMark:(TimeRulerMark*)timeMark;
//判断时间是否在时间轴里
+(NSString*)checkTime:(CGFloat)time InPrimary:(TimeMarkModel*)primaryModel;
//获取播放时间
+(NSString*)getTime:(CGFloat)time From:(NSString*)fTime;
//获取时间点
+(NSString*)getTime:(NSString*)time withValue:(CGFloat)value;
@end

NS_ASSUME_NONNULL_END
