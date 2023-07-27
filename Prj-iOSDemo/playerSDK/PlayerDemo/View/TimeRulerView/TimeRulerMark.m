//
//  TimeRulerMark.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TimeRulerMark.h"

@implementation TimeRulerMark
- (instancetype)init
{
    self = [super init];
    if (self)
    {
        self.markHeight = 30.0;
        self.markColor = [UIColor colorWithWhite:0.46 alpha:1.0];
        self.markFont = [UIFont systemFontOfSize:11.0];
        self.textColor = [UIColor colorWithWhite:0.46 alpha:1.0];
    }
    return self;
}
@end
