//
//  TimeRulerLayer.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "TimeRulerMark.h"
#import "TimeMarkModel.h"
NS_ASSUME_NONNULL_BEGIN
typedef NS_ENUM(NSInteger, RulerMarkFrequency)
{
    RulerMarkFrequency_hour      = 3600, //小时标记频率3600秒
    RulerMarkFrequency_minute_30 = 1800,  //5分钟标记频率
    RulerMarkFrequency_minute_10 = 600,  //5分钟标记频率
    RulerMarkFrequency_minute_5 = 300,  //5分钟标记频率
    RulerMarkFrequency_minute_1  = 60,  //1分钟标记评率
};
@interface TimeRulerLayer : CALayer

@property(nonatomic,strong) TimeRulerMark* minorMark;

@property(nonatomic,strong) TimeRulerMark* middleMark;

@property(nonatomic,strong) TimeRulerMark* majorMark;

@property (nonatomic, strong) NSMutableArray* selectedTimeModelsArr;

@property (nonatomic, strong) TimeMarkModel* primaryModel;//时间轴基本设置

@property (nonatomic, assign) CGFloat minWidth;// 单位长度

@property (nonatomic, strong) NSMutableArray*  alarmModelsArr;

@property (nonatomic, assign) BOOL amplify;

@property (nonatomic, assign) CGFloat lastPosition;

@property (nonatomic, assign) RulerMarkFrequency  markFrequency;

-(void)setData:(NSMutableArray*)timeModeArr;
-(void)setAlarmData:(NSMutableArray*)alarmModeArr;
-(void)upadateScaleData:(BOOL)amplify;
-(CGFloat)getMarkFrequency;
@end

NS_ASSUME_NONNULL_END
