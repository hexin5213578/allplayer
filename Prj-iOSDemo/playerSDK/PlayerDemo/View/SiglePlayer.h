//
//  SiglePlayer.h
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/18.
//

#import <Foundation/Foundation.h>
#import "openGL20.h"
#import "TimeMarkModel.h"
NS_ASSUME_NONNULL_BEGIN

@interface SiglePlayer : NSObject

@property (nonatomic, strong) TimeMarkModel* primaryModel;

-(void)playerData:(NSDictionary*)playDic width:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done;

-(void)defaultPlaywidth:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done;

-(OpenGLView20*)getPlayerView;

-(void)realPlay;
-(void)realResume;
-(void)realStop;

-(void)videoPlayerData:(NSDictionary*)playDic width:(CGFloat)width done:(void (^)(OpenGLView20* playew,NSArray* videoArr, BOOL done))done;
-(void)recordPlay;
-(void)recordStop;
- (void)palyAtPoint:(double)start WithSpeed:(double)speed;
-(void)voiceSetting:(int)value;
-(void)videoRecord;
-(void)stopVideoRexord;
-(void)snapShot;

-(void)loacalPlay:(NSString*)path  width:(CGFloat)width done:(void (^)(OpenGLView20* playew, BOOL done))done;
-(void)loacalPlay;
-(void)localStop;


@end

NS_ASSUME_NONNULL_END
