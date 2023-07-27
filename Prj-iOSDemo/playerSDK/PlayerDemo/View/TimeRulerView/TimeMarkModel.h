//
//  TimeMarkModel.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN

@interface TimeMarkModel : NSObject



@property(nonatomic, strong)NSString* nearLabTime;//距离最近的刻度点

@property(nonatomic, strong)NSString* startTime;//标记开始时间

@property(nonatomic, assign)CGFloat startSec;

@property(nonatomic, strong)NSString* endTime;//标记结束时间

@property(nonatomic, assign)CGFloat endSec;

@property(nonatomic,strong) UIColor *markColor;//标记颜色

@property(nonatomic,strong)NSString* farLabTime;//结束距离最远刻度点

@property(nonatomic,assign)CGFloat totalTime;

@property (nonatomic, assign) CGRect startLabelRect;
@property (nonatomic, strong) NSString* startStr;


@end

NS_ASSUME_NONNULL_END
