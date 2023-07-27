//
//  OpenGLView20.h
//  MyTest
//
//  Created by smy  on 12/20/11.
//  Copyright (c) 2011 ZY.SYM. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface OpenGLView20 : UIView

#pragma mark - 接口

- (bool)Init;

- (void)Draw;

- (void)Close;

/**
 清除画面
 */
- (void)ClearFrame;

@end
