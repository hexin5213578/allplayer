//
//  TimeRulerMark.h
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
typedef NS_ENUM(NSInteger, RulerMarkType)
{
    RulerMarkType_min_10 = 600 ,
    RulerMarkType_min_5 = 300,
    RulerMarkType_min = 60 ,
};

NS_ASSUME_NONNULL_BEGIN

@interface TimeRulerMark : NSObject
@property (nonatomic, assign) RulerMarkType markType;
@property (nonatomic, assign) CGSize markSize;
@property (nonatomic, assign) CGFloat markHeight;
@property (nonatomic, strong) UIColor * markColor;
@property (nonatomic, assign) UIFont* markFont;
@property (nonatomic, strong) UIColor * textColor;
@end

NS_ASSUME_NONNULL_END
