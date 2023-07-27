//
//  ACTimeRuler.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "TimeMarkModel.h"
#import "TimeRulerLayer.h"
NS_ASSUME_NONNULL_BEGIN

@protocol ACTimeRulerDelegate <NSObject>

@optional
//开始拖拽时间点
-(void)TimeRulerDragBeginAt:(CGFloat)timePointx;
//拖拽结束偏移时间
-(void)TimeRulerDragEndOffset:(CGFloat)timeOffset;

//开始拖拽
-(void)TimeRulerDragBegin;
//拖拽结束
-(void)TimeRulerDragEnd;
//当前播放时移
-(void)PlaybackTime:(CGFloat)time;
@end

@interface ACTimeRuler : UIView

@property (nonatomic, strong) UIView* lineView;

@property (nonatomic, strong) TimeRulerLayer* rulerLayer;//数据绘制层

@property (nonatomic, weak) id<ACTimeRulerDelegate> delegate;

@property (nonatomic, strong) NSMutableArray* selectedTimeModelsArr;//绘制区域数据

@property (nonatomic, strong) UIScrollView* scrollView;//可滑动标尺

@property (nonatomic, strong) UIView* referenceLine;//标尺指示线


-(void)updateTimeRuler:(CGFloat)time;
//更新时刻
-(void)updateTimeRulerAtTime:(NSString*)time;
//偏移时间
-(CGFloat)getTimeRulerOffset;
//切换镜头设置进度条数据
//-(void)setData:(NSMutableArray*)arr With:(TimeMarkModel*)primaryModel;
-(void)setData:(NSMutableArray*)arr With:(TimeMarkModel*)primaryModel PlayModel:(RecordList *)model;
-(void)updateAlarmData:(NSMutableArray*)arr;
@end

NS_ASSUME_NONNULL_END
