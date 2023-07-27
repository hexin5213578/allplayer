//
//  VideoPanelView.h
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import <UIKit/UIKit.h>
#import "CommonType.h"
NS_ASSUME_NONNULL_BEGIN
typedef void (^FullBlock)(BOOL full);
typedef void (^MultiBlock)(BOOL multiplay);
typedef void (^VoiceSettingBlock)(BOOL login);
typedef void (^PlayBlock)(BOOL play);
typedef void (^ActionBlock)(ActionType actionType,BOOL status);
@interface VideoPanelView : UIView
@property (readwrite, nonatomic, copy) FullBlock fullBlock;
@property (readwrite, nonatomic, copy) MultiBlock multiBlock;
@property (readwrite, nonatomic, copy) VoiceSettingBlock voiceSettingBlock;
@property (readwrite, nonatomic, copy) PlayBlock playBlock;
@property (readwrite, nonatomic, copy) ActionBlock actionBlock;
-(void)multiStatus:(BOOL)multi;
-(void)fullStatus:(BOOL)full;
-(void)playStatus:(BOOL)play;
-(void)action:(ActionType)actionType value:(BOOL)status;
@end

NS_ASSUME_NONNULL_END
