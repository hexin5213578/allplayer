//
//  HomePlayerView.h
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import <UIKit/UIKit.h>
#import "VideoPanelView.h"
#import "CommonType.h"
#import "SiglePlayer.h"

NS_ASSUME_NONNULL_BEGIN

@class  ViewController;

@interface HomePlayerView : UIView

@property (nonatomic, assign) PlayerViewType  playerviewType;

@property (nonatomic, strong) UIViewController* parentVC;

@property (nonatomic, strong) SiglePlayer* siglePlayer;

- (instancetype)initWithType:(PlayerViewType)playerviewType;

-(void)settingConfig:(BOOL)congfig;

-(void)selectDevice:(NSDictionary*)deviceDic;

-(void)selectVideo:(NSString*)path;
@end

NS_ASSUME_NONNULL_END
