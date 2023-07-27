//
//  TimeRulerLayer.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimeRulerLayer.h"
#import "TimeTool.h"


@implementation TimeRulerLayer
- (instancetype)init
{
    self = [super init];
    if (self)
    {
        self.primaryModel = [[TimeMarkModel alloc]init];
        self.primaryModel.startSec = 0;
        self.primaryModel.endSec = 3600*24;
        self.primaryModel.markColor = [UIColor darkGrayColor];
        self.minWidth = 10.0;
        self.lastPosition = 0;
        self.markFrequency = RulerMarkFrequency_minute_1;
    }
    return self;
}
-(TimeRulerMark*)minorMark{
    if (!_minorMark) {
        _minorMark = [[TimeRulerMark alloc]init];
        _minorMark.markType = RulerMarkType_min;
        _minorMark.markSize = CGSizeMake(1.0, 4.0);
    }
    return _minorMark;
}
-(TimeRulerMark*)middleMark{
    if (!_middleMark) {
        _middleMark = [[TimeRulerMark alloc]init];
        _middleMark.markType = RulerMarkType_min_5;
        _middleMark.markSize = CGSizeMake(1.0, 6.0);
    }
    return _middleMark;
}
-(TimeRulerMark*)majorMark{
    if (!_majorMark) {
        _majorMark = [[TimeRulerMark alloc]init];
        _majorMark.markType = RulerMarkType_min_10;
        _majorMark.markSize = CGSizeMake(1.0, 8.0);
    }
    return _majorMark;
}
-(void)setData:(NSMutableArray*)timeModeArr{
    self.selectedTimeModelsArr = timeModeArr;
    self.alarmModelsArr = [[NSMutableArray alloc]init];
    [self drawToImage];
}
-(void)setAlarmData:(NSMutableArray*)alarmModeArr{
    self.alarmModelsArr = alarmModeArr;
    [self drawToImage];
}
-(void)upadateScaleData:(BOOL)amplify{
    self.amplify = amplify;
    self.lastPosition = 0;
    
   [self drawToImage];
}
-(CGFloat)getMarkFrequency{
    CGFloat frequencyValue = (CGFloat)self.markFrequency;
    return frequencyValue;
}
-(void)upadateMarkFrequency:(RulerMarkFrequency)markFrency{
    self.markFrequency = markFrency;
}
-(void)display{
    [self drawToImage];
}
-(void)drawToImage{
  //  int frequency = 1;//十分钟一个单位
//    1min一个单位
//    5min一个单位
//    10min一个单位
//    30min一个单位
//    1个小时一个单位
    //尺长
    CGFloat rulerWith = (self.bounds.size.width);
    //单位长度设置
    CGFloat lineoffset = self.minWidth;
    int numberline = rulerWith/self.minWidth;
    
    UIGraphicsBeginImageContextWithOptions(self.frame.size, NO, [[UIScreen mainScreen] scale]);
    CGContextRef context = UIGraphicsGetCurrentContext();
    NSAttributedString* attributeString =  [[NSAttributedString alloc] initWithString:@"00:00" attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:11]}];
    CGFloat hourTextWith = attributeString.size.width;

    //绘制选中的区域
    for (TimeMarkModel* model in self.selectedTimeModelsArr ) {
        CGContextSetFillColorWithColor(context, model.markColor.CGColor);
        int startSec = model.startSec;
        int endSec = model.endSec;
        CGFloat x = (startSec)/(self.primaryModel.totalTime)*(numberline*lineoffset);
        CGFloat width = (endSec - startSec)/(self.primaryModel.totalTime)*(numberline*lineoffset);
        CGContextFillRect(context, CGRectMake(x, 0, width, self.bounds.size.height - self.minorMark.markHeight ));
    }
    //绘制告警的区域
    for(TimeMarkModel* model in self.alarmModelsArr){
        CGContextSetFillColorWithColor(context, model.markColor.CGColor);
        int startSec = model.startSec;
        int endSec = model.endSec;
        CGFloat x = (startSec)/(self.primaryModel.totalTime)*(numberline*lineoffset);
        CGFloat width = (endSec - startSec)/(self.primaryModel.totalTime)*(numberline*lineoffset);
        CGContextFillRect(context, CGRectMake(x, 0, width, self.bounds.size.height - self.minorMark.markHeight ));
    }
    //计算第一个刻度点
   
    
    //绘制每个标记的属性
    for(int i = 0 ; i < numberline;i++){
        NSString* markText = @"00:00";
        CGFloat position = (CGFloat)i * lineoffset  ;
        TimeRulerMark* mark = [[TimeRulerMark alloc]init];
        markText = [TimeTool getMarkStrAtTime:self.primaryModel.nearLabTime offsetTime:i*60.0 TimeMark:mark];
        switch (mark.markType) {
            case RulerMarkType_min_10:
                mark = self.majorMark;
                break;
            case RulerMarkType_min_5:
                mark = self.middleMark;
                break;
            case RulerMarkType_min:
                mark = self.minorMark;
                break;
            default:
                break;
        }
        [self drawMarkContext:context Position:position WithText:markText markType:mark];
    }
    
    
    UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    self.contents = (__bridge id _Nullable)([image CGImage]);
    self.frame = CGRectMake(0, 0, numberline*lineoffset, self.frame.size.height);
    
}

//绘制刻度
-(void)drawMarkContext:(CGContextRef)context Position:(CGFloat)position WithText:(NSString*)text markType:(TimeRulerMark*)mark{
    NSAttributedString* attributeString =  [[NSAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName:mark.markFont, NSForegroundColorAttributeName:mark.markColor}];
    CGSize markSize = mark.markSize;
    BOOL showText = NO;
    BOOL showMark = YES;
    switch (mark.markType) {
        case RulerMarkType_min_10:
            showText = YES;
            break;
        case RulerMarkType_min_5:
            showText = YES;
            break;
        case RulerMarkType_min:
            showText = NO;
            break;
            
        default:
            break;
    }

    showMark = YES;
    self.lastPosition = position;
    // 绘制刻度尺
    if (showMark) {
        CGFloat rectX = position - markSize.width*0.5;
        CGFloat rectY = 0;
        CGContextRef ctx = UIGraphicsGetCurrentContext();
        CGContextSetStrokeColorWithColor(ctx, self.primaryModel.markColor.CGColor);
        CGContextSetLineWidth(ctx, 1);
        CGContextMoveToPoint(ctx, rectX, self.bounds.size.height + markSize.height - mark.markHeight );
        CGContextAddLineToPoint(context, rectX, self.bounds.size.height - 1- mark.markHeight);
        CGContextStrokePath(context);
    }
    //绘制时间文字
    if (showText) {
        CGSize textSize = [attributeString size];
        CGFloat textRectX= position - textSize.width * 0.5;
        CGFloat textRectY = self.bounds.size.height - mark.markHeight/3*2;
        CGRect textRect = CGRectMake(textRectX, textRectY, textSize.width , textSize.height);
        if (position == 0) {
            self.primaryModel.startLabelRect = textRect;
            self.primaryModel.startStr = text;
        }
        if (position != 0) {
            [text drawInRect:textRect withAttributes:@{NSFontAttributeName:mark.markFont, NSForegroundColorAttributeName:mark.markColor}];
        }
        
    }
}
@end
