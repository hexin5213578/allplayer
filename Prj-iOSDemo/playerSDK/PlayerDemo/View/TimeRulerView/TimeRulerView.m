//
//  TimeRulerView.m
//  ChinaMobileProject
//  进度刻度尺
//  Created by 秦骏 on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimeRulerView.h"
#import "Masonry.h"
#import "TimeTool.h"
@interface TimeRulerView()<ACTimeRulerDelegate>
@property(nonatomic,assign)bool isChecked;
@end
@implementation TimeRulerView


-(instancetype)initWithFrame:(CGRect)frame
{
    if(self = [super initWithFrame:frame])
    {
        [self setBackgroundColor:[UIColor whiteColor]];
        self.acTimeRuler.frame = self.bounds;
        [self addSubview:self.acTimeRuler];
        [self.acTimeRuler mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.mas_top);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.bottom.equalTo(self.mas_bottom);
        }];
        self.playVideoManager = [[TimePlayManager alloc]init];
        self.rulerPlayType = RulerPlayType_Normal;
        self.isChecked = NO;
    }
    return self;
}
-(ACTimeRuler*)acTimeRuler{
    if (!_acTimeRuler) {
        _acTimeRuler = [[ACTimeRuler alloc]init];
        _acTimeRuler.delegate = self;
    }
    return _acTimeRuler;
}

-(void)initRulerViewWith:(NSArray*)videArr withPraModel:(TimeMarkModel*)mark{
    NSMutableArray* markArr = [[NSMutableArray alloc]init];
    for (NSDictionary *model in videArr) {
        TimeMarkModel* remarkmodel = [[TimeMarkModel alloc]init];
        remarkmodel.markColor =  UIColorFromHex(0xFF669900);
        remarkmodel.startTime = [model objectForKey:@"beginTime"] ;
        remarkmodel.endTime = [model objectForKey:@"endTime"];
        remarkmodel.startSec = [TimeTool getIntervalfTime:[model objectForKey:@"beginTime"]  since:mark.nearLabTime];
        remarkmodel.endSec = [TimeTool getIntervalfTime:[model objectForKey:@"endTime"]  since:mark.nearLabTime];
        [markArr addObject:remarkmodel];
    }
    self.primaryModel  = mark;
    self.videoArr = videArr;
    self.currentPlayIndex = 0;
    self.currentPlayRecord =[videArr objectAtIndex:self.currentPlayIndex];
    [self.acTimeRuler setData:markArr With:mark PlayModel:self.currentPlayRecord];
    __weak typeof(self) weakSelf = self;
    [self.playVideoManager getPlayUrlwithRecordInfo:self.currentPlayRecord finishBlock:^(NSString * _Nonnull playUrl) {
      
        if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(playRecordWith:cameraModel:)])
        {
            if (!weakSelf.currentCamera) {
                weakSelf.currentCamera = [weakSelf.cameraArr objectAtIndex:weakSelf.currentCameraIndex];
            }
            [weakSelf.delegate playRecordWith:playUrl cameraModel:weakSelf.currentCamera];
        }
    }];
}
-(void)updateRulerViewWithAlarmArr:(NSMutableArray*)alarmArr{
    NSMutableArray* markArr = [[NSMutableArray alloc]init];
    for (NSDictionary * alarmmode in alarmArr) {
        TimeMarkModel* remarkmodel = [[TimeMarkModel alloc]init];
        remarkmodel.markColor =  [UIColor redColor];
        remarkmodel.startTime =  [TimeTool getTime:[alarmmode  objectForKey:@"alarmTime"]  withValue:-30]; //向前30s
        remarkmodel.endTime =  [TimeTool getTime:[alarmmode  objectForKey:@"alarmTime"] withValue:30]; //向后30s
        remarkmodel.startSec = [TimeTool getIntervalfTime: remarkmodel.startTime since:self.primaryModel.nearLabTime];
        remarkmodel.endSec = [TimeTool getIntervalfTime: remarkmodel.endTime since:self.primaryModel.nearLabTime];
        [markArr addObject:remarkmodel];
    }
    [self.acTimeRuler updateAlarmData:markArr];
}

-(void)initRulerViewCarmeraArr:(NSArray*)arr fromStartTime:(NSString*)startTime toEndTime:(NSString*)endTime withForm:(NSString*)form {
    self.form = form;
    self.cameraArr = arr;
    self.primaryModel = [TimeTool getPrimaryModelfromsTime:startTime toeTime:endTime];
    
    self.currentCameraIndex = 0;
    NSDictionary* cameraModel = [arr objectAtIndex:self.currentCameraIndex];
    self.rulerPlayType = RulerPlayType_RoundCamera;
    __weak typeof(self) weakSelf = self;
    [self.playVideoManager getVideoArrWithCameraId:[cameraModel objectForKey:@"deviceId"] StartTime:startTime EndTime:endTime withForm:form finishBlock:^(NSMutableArray * _Nonnull videoArr,NSError * error) {
        if (videoArr.count > 0) {
            [weakSelf initRulerViewWith:videoArr withPraModel:weakSelf.primaryModel];
        }else{
            if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(viewShowTips:)])
            {
                [weakSelf.delegate viewShowTips:NSLocalizedString(@"tip_no_videoData", nil)];
            }
        }
       
    }];
}

//查找数据

-(void)findPlayRecord:(NSDictionary *)recordmodel PlayUrlfinishBlock:(void(^)(NSDictionary *model,NSString* playurl))finishBlock{
    
    [self.playVideoManager getPlayUrlwithRecordInfo:recordmodel finishBlock:^(NSString * _Nonnull playUrl) {
        finishBlock(recordmodel,playUrl);
    }];
}
-(void)findPlayVideoArrfinishBlock:(void(^)(NSDictionary* cameraModel ,NSMutableArray* videoArr))finishBlock{
    if (self.rulerPlayType == RulerPlayType_RoundCamera) {
        NSDictionary* cameraModel  =  [self.cameraArr objectAtIndex:self.currentCameraIndex];
        [self.playVideoManager getVideoArrWithCameraId:[cameraModel objectForKey:@"deviceId"] StartTime:self.primaryModel.startTime EndTime:self.primaryModel.endTime withForm:self.form finishBlock:^(NSMutableArray * _Nonnull videoArr,NSError * error) {
            finishBlock(cameraModel,videoArr);
        }];
    }else{
        NSMutableArray* videoAr = [[NSMutableArray alloc]initWithArray:self.videoArr];
        finishBlock(self.currentCamera,videoAr);
    }
}
-(void)findPlayDataWith:(NSString*)time finishBlock:(void(^)(NSInteger findIndex,CGFloat packtime,NSString* playurl))finishBlock{
   
    
    NSDictionary* precordModel = [[NSDictionary alloc]init];
    NSInteger findIndex = -1;
    for (NSDictionary* recordModel in self.videoArr) {
        CGFloat intervalsTime =  [TimeTool getIntervalfTime:time since:[recordModel objectForKey:@"beginTime"] ];
        CGFloat intervaleTime =  [TimeTool getIntervalfTime:time since:[recordModel objectForKey:@"endTime"]];
        if (intervalsTime > 0 && intervaleTime < 0 ) {
            precordModel = recordModel;
            findIndex = [self.videoArr indexOfObject:recordModel];
            break;
        }
    }
    if (findIndex != -1) {
        
        if ([ [self.currentPlayRecord objectForKey:@"beginTime"] isEqualToString:[precordModel objectForKey:@"beginTime"]]) {
            CGFloat packbacktime = [TimeTool getIntervalfTime:time since:[self.currentPlayRecord objectForKey:@"beginTime"]];
            finishBlock(findIndex,packbacktime,@"");
        }else{
          
            __weak typeof(self) weakSelf = self;
            [self findPlayRecord:precordModel PlayUrlfinishBlock:^(NSDictionary *model,NSString *playurl) {
                weakSelf.currentPlayRecord = model;
                CGFloat packbacktime = [TimeTool getIntervalfTime:time since:[model objectForKey:@"beginTime"]];
                finishBlock(findIndex,packbacktime,playurl);
            }];
        }

    }else{
        finishBlock(findIndex,0,@"");
    }
}
-(void)checkPlayData:(double)speed finishBlock:(void(^)(bool isMore,NSString* playurl,CGFloat startTime,NSDictionary* cameraModel ))finishBlock{
    switch (self.rulerPlayType) {
        case RulerPlayType_Normal:
        {
            if (speed > 0) {
                if (self.currentPlayIndex < self.videoArr.count - 1) {
                    self.currentPlayIndex = self.currentPlayIndex + 1;
                }else
                {
                    //没有更多播放数据
                    finishBlock(NO,@"",0,self.currentCamera);
                    return;
                }
            }else{
                if (self.currentPlayIndex > 0) {
                    self.currentPlayIndex = self.currentPlayIndex - 1;
                }else
                {
                    //更多的播放数据
                    finishBlock(NO,@"",0,self.currentCamera);
                    return;
                }
            }
        }
            break;
            
        case RulerPlayType_RoundCamera:
        {
            if (self.currentPlayIndex < self.videoArr.count - 1) {
                self.currentPlayIndex = self.currentPlayIndex + 1;
            }else{
             
                self.currentPlayIndex = 0;
                if (self.currentCameraIndex < self.cameraArr.count - 1) {
                    self.currentCameraIndex = self.currentCameraIndex + 1;
                }
            }
        }
            break;
        default:
            break;
    }
    //获取播放地址
    __weak typeof(self) weakSelf = self;
    [self findPlayVideoArrfinishBlock:^(NSDictionary* cameraModel ,NSMutableArray *videoArr) {
        weakSelf.videoArr = videoArr;
        __weak typeof(self) weakSelfa = weakSelf;
        NSDictionary *recordmodel = [videoArr objectAtIndex:self.currentPlayIndex];
        [weakSelf findPlayRecord:recordmodel PlayUrlfinishBlock:^(NSDictionary *model,NSString *playurl) {
            weakSelfa.currentPlayRecord = model;
            NSDictionary * dic = @{@"currentLoopCameraIndex":[NSString stringWithFormat:@"%ld",weakSelfa.currentCameraIndex],@"videoLoopFail":@"0"};
            [[NSNotificationCenter defaultCenter] postNotificationName:@"changeLoopStatus" object:dic];
            if (speed > 0 ) {
                finishBlock(YES,playurl,0,cameraModel);
            }else{
                CGFloat totalTime = [TimeTool getIntervalfTime:[model objectForKey:@"endTime"] since:[model objectForKey:@"beginTime"]];
                self.hasStartTime = YES;
                finishBlock(YES,playurl,totalTime,cameraModel);
            }

        }];
    }];
    
}
-(void)updateRulerViewWithCarmeraId:(NSString*)cameraId fromStartTime:(NSString*)startTime toEndTime:(NSString*)endTime{
    
}
//一个时间段，多个镜头，多个录像数据
-(void)initRulerViewWithDuration:(TimeMarkModel*)markModel{
    
}
//用偏移量
-(void)updateRulerPlayTimeOffset:(CGFloat)time{
  //  [self.acTimeRuler updateTimeRuler:time];
    
    NSString* atime = [TimeTool getTime:time From:[self.currentPlayRecord objectForKey:@"beginTime"]];
    [self.acTimeRuler updateTimeRulerAtTime:atime];
}
//开始拖拽时间点
-(void)TimeRulerDragBeginAt:(CGFloat)timePointx{
    NSLog(@"TimeRulerDragBeginAt%f",timePointx);
}
//拖拽结束偏移时间
-(void)TimeRulerDragEndOffset:(CGFloat)timeOffset{
    NSLog(@"TimeRulerDragEndOffset%f",timeOffset);
}

//开始拖拽
-(void)TimeRulerDragBegin{
    if (self.delegate && [self.delegate respondsToSelector:@selector(dragTimeRulerBeign)])
    {
        [self.delegate dragTimeRulerBeign];
    }
    
}
//拖拽结束
-(void)TimeRulerDragEnd{
    
    if (self.isChecked == YES) {
        return;
    }
    self.isChecked = YES;
    //查验一些该节点在不在播放范围内
    CGFloat playTime = [self.acTimeRuler getTimeRulerOffset];
    NSString* checkTime = [TimeTool checkTime:playTime InPrimary:self.primaryModel];
    
    if ([checkTime isEqualToString:@"-1"]) {
        //1 当前播放
        [self.delegate rulerToastTips:@"该时间点没有录像"];
        self.isChecked = NO;
    }else{
        //2 不在该范围内寻找录像时段
        __weak typeof(self) weakSelf = self;
        [self findPlayDataWith:checkTime finishBlock:^(NSInteger findIndex, CGFloat playbacktime,NSString *playurl) {
            if (findIndex  != -1) {
                //计算下一段的starttime
                if ([playurl isEqualToString:@""]) {
                    if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(dragTimeRulerPlayWithOffserTime:)])
                    {
                        weakSelf.hasStartTime = YES;
                        [weakSelf.delegate dragTimeRulerPlayWithOffserTime:playbacktime];
                    }
                }else{
                    if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(dragTimeRulerPlayWithOffserTime:withUrl:)])
                    {
                        [weakSelf.delegate dragTimeRulerPlayWithOffserTime:playbacktime withUrl:playurl];
                        self.currentPlayIndex = findIndex;
                        weakSelf.hasStartTime = YES;
                    }
                }
               
            }else{
                if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(rulerToastTips:)])
                {
                    [weakSelf.delegate rulerToastTips:@"该时间点没有录像"];
                }
            }
            weakSelf.isChecked = NO;
        }];
    }
    
}


@end
