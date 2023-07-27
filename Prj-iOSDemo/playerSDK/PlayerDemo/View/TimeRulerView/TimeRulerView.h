//
//  TimeRulerView.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "TimePlayManager.h"
#import "ACTimeRuler.h"
#import "TimeMarkModel.h"

NS_ASSUME_NONNULL_BEGIN
typedef NS_ENUM(NSInteger, TimeRulerPlayType)
//enum PlaySpeed
{
    RulerPlayType_Normal = 0,// 单镜头播放 请求的的url 自动播放下一个
    RulerPlayType_RoundCamera = 1,//多镜头轮巡播放
};

@protocol TimeRulerViewDelegate <NSObject>

-(void)dragTimeRulerBeign;

-(void)dragTimeRulerPlayWithOffserTime:(CGFloat)playTime withUrl:(NSString*)url;

-(void)playRecordWith:(NSString*)url cameraModel:(NSDictionary*)mode;

-(void)rulerToastTips:(NSString*)tips;

-(void)viewShowTips:(NSString*)tips;

-(void)dragTimeRulerPlayWithOffserTime:(CGFloat)playTime;

@end
@interface TimeRulerView : UIView

@property (nonatomic,strong) TimePlayManager* playVideoManager;//录像时间数据管理

@property (nonatomic, strong) ACTimeRuler* acTimeRuler;//时间轴

@property (nonatomic, weak) id<TimeRulerViewDelegate> delegate;

@property (nonatomic, strong) NSDictionary* currentPlayRecord;
@property (nonatomic, assign) NSInteger currentPlayIndex;

@property (nonatomic, strong) NSDictionary* currentCamera;
@property (nonatomic, assign) NSInteger currentCameraIndex;

@property (nonatomic, strong) NSArray* cameraArr;
@property (nonatomic, strong) NSArray* videoArr;

@property (nonatomic, strong) TimeMarkModel* primaryModel;
@property (nonatomic, strong) NSString* form;

@property (nonatomic, assign) TimeRulerPlayType rulerPlayType;

@property (nonatomic, assign) BOOL hasStartTime;//

//切换镜头
-(void)updateRulerViewWithCarmeraId:(NSString*)cameraId fromStartTime:(NSString*)startTime toEndTime:(NSString*)endTime;
//录像数据
-(void)initRulerViewWith:(NSArray*)videArr withPraModel:(TimeMarkModel*)mark;
//轮巡镜头
-(void)initRulerViewCarmeraArr:(NSArray*)arr fromStartTime:(NSString*)startTime toEndTime:(NSString*)endTime withForm:(NSString*)form;
//自动播放数据
-(void)checkPlayData:(double)speed finishBlock:(void(^)(bool isMore,NSString* playurl,CGFloat startTime,NSDictionary* cameraModel ))finishBlock;

-(void)initRulerViewWithDuration:(TimeMarkModel*)markModel;
//用偏移量
-(void)updateRulerPlayTimeOffset:(CGFloat)time;

-(void)updateRulerViewWithAlarmArr:(NSMutableArray*)alarmArr;
@end

NS_ASSUME_NONNULL_END
