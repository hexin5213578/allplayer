//
//  ACTimeRuler.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "ACTimeRuler.h"
#import "Masonry.h"
#import "TimeTool.h"

static CGFloat const rulerMaxWidth = 10800.0;
@interface ACTimeRuler()<UIScrollViewDelegate>
@property(nonatomic,assign) CGFloat dragX;
@property(nonatomic,assign) CGFloat offsetW;
@property(nonatomic,assign) CGFloat currentPx;
@property(nonatomic,assign) CGFloat originOffset;//位置偏移
@property(nonatomic,assign) CGFloat actOffset;//显示偏移
@property(nonatomic,assign) CGFloat startScale;//初始比例

@property(nonatomic,assign) CGFloat totalTime;
@property(nonatomic,assign) CGFloat currentTime;

@property(nonatomic,strong) UILabel* currentTimeLab;

@property(nonatomic,assign) CGFloat oldRulerWidth;//原尺的长度
@property(nonatomic,assign) CGFloat rulerWidth;//尺长

@property (nonatomic, assign) bool isTouched;
@property (nonatomic, strong) UILabel* startLabel;
@property (nonatomic, strong) UIView* startMark;
@end
@implementation ACTimeRuler

-(instancetype)initWithFrame:(CGRect)frame
{
    if(self = [super initWithFrame:frame])
    {
        [self addSubview:self.scrollView];
        [self.scrollView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.mas_top);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.bottom.equalTo(self.mas_bottom).offset(-1.0);
        }];
        [self addSubview:self.lineView];
        [self.lineView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.height.mas_equalTo(1.0f);
            make.bottom.equalTo(self.mas_bottom).offset(-30.0);
        }];
        [self addSubview:self.referenceLine];
        [self.referenceLine mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerX.equalTo(self.mas_centerX);
            make.top.equalTo(self.mas_top);
            make.width.mas_equalTo(1.7f);
            make.bottom.equalTo(self.mas_bottom).offset(-30.0);
        }];
        [self addSubview:self.currentTimeLab];
        [self.currentTimeLab mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerX.equalTo(self.mas_centerX);
            make.centerY.equalTo(self.mas_centerY);
            make.width.mas_equalTo(300.0f);
            make.height.mas_equalTo(30.0f);
        }];
        [self.scrollView.layer addSublayer:self.rulerLayer];
        [self.scrollView addSubview:self.startLabel];
        [self.scrollView addSubview:self.startMark];
        
        self.dragX = 0;
        //暂不支持缩放
//        UIPinchGestureRecognizer *pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(pinchGesture:)];
//        //把tapGesture（手势）添加到对应的view
//        [self addGestureRecognizer:pinchGesture];
        self.totalTime = 0;
        self.isTouched = NO;
        self.startScale = 1;
    }
    return self;
}
-(void)pinchGesture:(UIPinchGestureRecognizer*)recoginer{
    if (recoginer.state == UIGestureRecognizerStateBegan) {
        self.startScale = recoginer.scale;
    }else if(recoginer.state == UIGestureRecognizerStateChanged){
        CGFloat kscale = recoginer.scale / self.startScale;
        [self updateFrame:kscale];
    }
}
-(void)updateFrame:(CGFloat)scale{
    
//    CGFloat updateRulerWith = self.rulerLayer.bounds.size.width * scale;
//    if (updateRulerWith < self.bounds.size.width) {
//        updateRulerWith = self.bounds.size.width;
//    }
//
//    if (updateRulerWith > rulerMaxWidth) {
//        updateRulerWith = rulerMaxWidth;
//    }
//
//    self.oldRulerWidth = self.rulerWidth;
//    self.rulerWidth = updateRulerWith;
//    MyLog(@"updateSacle scale：%f",scale);
//    MyLog(@"updateSacle O：%f",self.oldRulerWidth);
//    MyLog(@"updateSacle N：%f",self.rulerWidth);
    //改变刻度
    CGFloat freqency =  [self.rulerLayer getMarkFrequency];
    if (scale > 1.0) {
        //放大 频率等级调低
    }else{
        //
    }
    //改变刻度单位的长度 缩小刻度尺
    if (scale > 1.0) {
        [self updateSacle:YES];
    }else{
        [self updateSacle:NO];
    }
}
-(UILabel*)startLabel{
    if (!_startLabel) {
        _startLabel = [[UILabel alloc]initWithFrame:CGRectMake(-60, 70, 60, 30)];
        _startLabel.text = @"";
        _startLabel.font = self.rulerLayer.majorMark.markFont;
        _startLabel.textColor = self.rulerLayer.majorMark.markColor;
    }
    return _startLabel ;
}
-(UIView*)startMark{
    if (!_startMark) {
        _startMark = [[UIView alloc]initWithFrame:CGRectMake(-1, 70, 1, self.rulerLayer.majorMark.markSize.height)];
        _startMark.backgroundColor =  self.rulerLayer.majorMark.markColor;
    }
    return _startMark;
}

-(void)layoutSubviews{
    [super layoutSubviews];
    CGFloat sideInset = self.bounds.size.width/2.0;
    self.scrollView.frame = self.bounds;
    self.scrollView.contentInset = UIEdgeInsetsMake(0, sideInset, 0, sideInset);
    if (self.rulerLayer.primaryModel.startLabelRect.origin.x < 0) {
        _startLabel.frame = self.rulerLayer.primaryModel.startLabelRect;
        _startLabel.text = self.rulerLayer.primaryModel.startStr;
    }
    [CATransaction begin];
    [CATransaction setDisableActions:true];
    self.rulerLayer.frame = CGRectMake(0, 0, self.rulerWidth, self.bounds.size.height);
    [CATransaction commit];
    self.scrollView.contentSize = CGSizeMake(self.rulerWidth, self.bounds.size.height);
    if (self.rulerLayer.primaryModel.totalTime > 0) {
        self.scrollView.contentOffset = [self propconentOffset:self.currentTime];
    }
    
}
-(CGPoint)propconentOffset:(CGFloat)currentTime{
    
    CGFloat proportion =  currentTime / self.rulerLayer.primaryModel.totalTime;
    CGFloat proportionWith = self.scrollView.contentSize.width * proportion;
    CGPoint pPoint  = CGPointMake((proportionWith - self.scrollView.contentInset.left), self.scrollView.contentOffset.y);
    return  pPoint;
}
-(UILabel*)currentTimeLab{
    if (!_currentTimeLab) {
        _currentTimeLab = [[UILabel alloc]init];
        _currentTimeLab.textColor = self.rulerLayer.primaryModel.markColor;
        _currentTimeLab.font = self.rulerLayer.minorMark.markFont;
        _currentTimeLab.hidden = YES;
    }
    return _currentTimeLab;
}
-(TimeRulerLayer*)rulerLayer{
    if (!_rulerLayer) {
        _rulerLayer = [[TimeRulerLayer alloc]init];
    }
    return _rulerLayer;
}
-(UIView*)lineView{
    if (!_lineView) {
        _lineView = [[UIView alloc]init];
        _lineView.backgroundColor = self.rulerLayer.primaryModel.markColor;;
    }
    return _lineView;
}
-(UIScrollView*)scrollView
{
    if (!_scrollView) {
        _scrollView = [[UIScrollView alloc]init];
        _scrollView.showsHorizontalScrollIndicator = NO;
        _scrollView.scrollEnabled = YES;
        _scrollView.bounces = NO;
        _scrollView.alwaysBounceHorizontal = YES;
        _scrollView.directionalLockEnabled = YES;
        [_scrollView setDelegate:self];
        
    }
    return  _scrollView;
}
-(UIView*)referenceLine{
    if (!_referenceLine) {
        _referenceLine = [[UIView alloc]init];
        _referenceLine.backgroundColor = [UIColor systemBlueColor];
    }
    return _referenceLine;
}
-(void)setData:(NSMutableArray*)arr With:(TimeMarkModel*)primaryModel PlayModel:(NSDictionary *)model{
    self.rulerLayer.primaryModel = primaryModel;
    
    CGFloat freqency =  [self.rulerLayer getMarkFrequency];
    
    self.rulerWidth = (self.rulerLayer.primaryModel.totalTime /freqency) * self.rulerLayer.minWidth;
    self.rulerLayer.frame = CGRectMake(0, 0, self.rulerWidth, self.bounds.size.height);
    self.scrollView.contentSize = CGSizeMake(self.rulerWidth, 0);
    [self.rulerLayer setData:arr];
    self.currentTime = [TimeTool getIntervalfTime:[model objectForKey:@"beginTime"] since:primaryModel.nearLabTime];;
  
}
-(CGFloat)defaultValue{
    return 6.0* self.bounds.size.width;
}
-(void)updateAlarmData:(NSMutableArray*)arr{
    [self.rulerLayer setAlarmData:arr];
}
-(void)updateSacle:(BOOL)amplify{
    if (amplify) {
        self.rulerLayer.minWidth = 10.0;
    }else{
        self.rulerLayer.minWidth = 4.0;
    }
    CGFloat freqency =  [self.rulerLayer getMarkFrequency];
    self.rulerWidth = (self.rulerLayer.primaryModel.totalTime /freqency) * self.rulerLayer.minWidth;
    self.rulerLayer.frame = CGRectMake(0, 0, self.rulerWidth, self.bounds.size.height);
    self.scrollView.contentSize = CGSizeMake(self.rulerWidth, 0);
    
    [self.rulerLayer upadateScaleData:amplify];
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    CGFloat proptionWidth = self.scrollView.contentOffset.x + self.scrollView.contentInset.left;
    
    CGFloat proption = proptionWidth /(self.scrollView.contentSize.width);
    int value = proption * self.rulerLayer.primaryModel.totalTime;
    self.currentTime = value;
    
    NSString* cur = [TimeTool getTime:self.currentTime From:self.rulerLayer.primaryModel.nearLabTime];
    self.currentTimeLab.text = [NSString stringWithFormat:@"时间刻度%@进度%f",cur,self.currentTime];
}
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView{
    self.isTouched = YES;
    self.dragX = scrollView.contentOffset.x;
    NSLog(@"开始拖拽%f",self.dragX);
    if (self.delegate && [self.delegate respondsToSelector:@selector(TimeRulerDragBegin)])
    {
        [self.delegate TimeRulerDragBegin];
    }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate{
   CGFloat dragX2 = scrollView.contentOffset.x;
    if (decelerate) {
        //静止惯性移动
        dispatch_async(dispatch_get_main_queue(), ^{
            [scrollView setContentOffset:scrollView.contentOffset animated:NO];
        });
    }
    self.offsetW = dragX2 - self.dragX;
    NSLog(@"偏移量%f",self.offsetW);
   
    if (self.delegate && [self.delegate respondsToSelector:@selector(TimeRulerDragEnd)])
    {
        [self.delegate TimeRulerDragEnd];
    }
    self.isTouched = NO;
}

-(CGFloat)getcurrentTimeOffset
{
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDate *currentDate = [NSDate date];
    NSDateComponents *components = [calendar components:NSCalendarUnitYear|NSCalendarUnitMonth|NSCalendarUnitDay fromDate:currentDate];
    NSDate *zeroDate = [calendar dateFromComponents:components];
    NSDate *datenow = [NSDate date];
    NSTimeInterval time = [datenow timeIntervalSinceDate:zeroDate];
    
    CGFloat offset = (time/60.0)* self.rulerLayer.minWidth;
    return offset;
}
-(void)updateTimeRuler:(CGFloat)time{
    if (self.isTouched) {
        return;
    }
    self.currentTime =  self.rulerLayer.primaryModel.startSec + time;
    [self setNeedsLayout];
 
}
//更新时刻
-(void)updateTimeRulerAtTime:(NSString*)time{
    if (self.isTouched) {
        return;
    }
    CGFloat rulertime = [TimeTool getIntervalfTime:time since:self.rulerLayer.primaryModel.nearLabTime];
    self.currentTime = rulertime;
    [self setNeedsLayout];
}

-(CGFloat)getTimeRulerOffset{
    CGFloat time = self.currentTime;
    return time;
}
@end
