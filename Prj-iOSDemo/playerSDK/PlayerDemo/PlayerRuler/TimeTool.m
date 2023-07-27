//
//  TimeTool.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/10.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimeTool.h"
#import "Categories.h"
@implementation TimeTool
+(CGFloat)getcurrentTimeOffset:(CGFloat)unitWidth
{
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDate *currentDate = [NSDate date];
    NSDateComponents *components = [calendar components:NSCalendarUnitYear|NSCalendarUnitMonth|NSCalendarUnitDay fromDate:currentDate];
    NSDate *zeroDate = [calendar dateFromComponents:components];
    NSDate *datenow = [NSDate date];
    NSTimeInterval time = [datenow timeIntervalSinceDate:zeroDate];
    
    CGFloat offset = (time/60.0)* unitWidth;
    return offset;
}
+(DurationModel*)getCurrentTimeDefaultVideoDuration{
    DurationModel* durationMode = [[DurationModel alloc]init];
    NSDate *endDate = [NSDate date];
    CGFloat IntervalVal = 24*60*60;
    NSDate *startDate = [NSDate dateWithTimeIntervalSinceNow:-IntervalVal];
    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"YYYY-MM-dd HH:mm:ss"];
    NSString *endTimeString = [formatter stringFromDate:endDate];
    NSLog(@"endTimeString =  %@",endTimeString);
    
    NSString *startTimeString = [formatter stringFromDate:startDate];
    NSLog(@"endTimeString =  %@",startTimeString);
    
    durationMode.startTime = startTimeString;
    durationMode.endTime = endTimeString;
    return durationMode;
}
+(TimeMarkModel*)getPrimaryModelfromsTime:(NSString*)startTime toeTime:(NSString*)endTime{
    TimeMarkModel* markmodel = [[TimeMarkModel alloc]init];
    
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *startDate = [dateFormatter dateFromString:startTime];
    NSDate *endDate = [dateFormatter dateFromString:endTime];
   // NSTimeInterval time = [endDate timeIntervalSinceDate:startDate];
    NSDate *labDate = [startDate dateToNearestMinutes:20 isStart:YES];
    NSString *labTimeStr = [dateFormatter stringFromDate:labDate];
    markmodel.nearLabTime = labTimeStr;
    markmodel.startTime = startTime;
    markmodel.endTime = endTime;
    markmodel.startSec = [TimeTool getIntervalfTime:markmodel.startTime since:markmodel.nearLabTime];
    markmodel.endSec = [TimeTool getIntervalfTime:markmodel.endTime since:markmodel.nearLabTime];
    NSDate *farlabDate = [endDate dateToNearestMinutes:5 isStart:NO];
    NSString *farlabTimeStr = [dateFormatter stringFromDate:farlabDate];
    markmodel.farLabTime = farlabTimeStr;
    markmodel.markColor = [UIColor colorWithWhite:0.46 alpha:1.0];
    markmodel.totalTime = [TimeTool getIntervalfTime:farlabTimeStr since:labTimeStr];
    return markmodel;
}
+(CGFloat)getIntervalfTime:(NSString*)cTime since:(NSString*)oTime{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *oTimeDate = [dateFormatter dateFromString:oTime];
    NSDate *cTimeDate = [dateFormatter dateFromString:cTime];
    NSTimeInterval intervalTime = [cTimeDate timeIntervalSinceDate:oTimeDate];
    return intervalTime;
}
//获得偏移标签
+(NSString*)getMarkStrAtTime:(NSString*)startTime offsetTime:(CGFloat)time TimeMark:(TimeRulerMark*)timeMark{
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *labStartData = [dateFormatter dateFromString:startTime];
    
    NSDate * markDate =  [NSDate dateWithTimeInterval:time sinceDate:labStartData];
   // NSDate *markDate = [NSDate dateWithTimeIntervalSinceNow:time];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm"];
    NSString *markStr = [formatter stringFromDate:markDate];
    
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDateComponents *components = [calendar components:(NSCalendarUnitHour | NSCalendarUnitMinute) fromDate:markDate];
    //NSInteger hour = [components hour];
    NSInteger minute = [components minute];
    
    timeMark.markType = RulerMarkType_min;
    
    NSInteger d5 = minute%5;
    if (d5 == 0) {
        timeMark.markType = RulerMarkType_min_5;
    }
    NSInteger d10 = minute%10;
    if (d10 == 0) {
        timeMark.markType = RulerMarkType_min_10;
    }
    return markStr;
}
//判断时间是否在时间轴里
+(NSString*)checkTime:(CGFloat)time InPrimary:(TimeMarkModel*)primaryModel{
    NSString* labTime = primaryModel.nearLabTime;
    NSString* playStartTime = primaryModel.startTime;
    NSString* playEndTime = primaryModel.endTime;
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *labData = [dateFormatter dateFromString:labTime];
    NSDate * willData =  [NSDate dateWithTimeInterval:time sinceDate:labData];
    
    NSDate *playEndData = [dateFormatter dateFromString:playEndTime];
    NSDate *palyStartData = [dateFormatter dateFromString:playStartTime];

    NSString *willTime = [dateFormatter stringFromDate:willData];
    
    NSTimeInterval intervalsTime = [willData timeIntervalSinceDate:palyStartData];
    NSTimeInterval intervaleTime = [willData timeIntervalSinceDate:playEndData];
    
    if (intervalsTime > 0  && intervaleTime < 0) {
        return willTime;//在时间轴上
    }else{
        return @"-1";//存在当前播放路线时段
    }
}
+(NSString*)getTime:(CGFloat)time From:(NSString*)fTime{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *fData = [dateFormatter dateFromString:fTime];
    NSDate * timeData =  [NSDate dateWithTimeInterval:time sinceDate:fData];
    NSString *timeStr = [dateFormatter stringFromDate:timeData];
    return timeStr;
}
+(NSString*)getTime:(NSString*)time withValue:(CGFloat)value{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSDate *timeDate = [dateFormatter dateFromString:time];
    NSDate * willData =  [NSDate dateWithTimeInterval:value sinceDate:timeDate];
    NSString *timeStr = [dateFormatter stringFromDate:willData];
    return timeStr;
}
@end
