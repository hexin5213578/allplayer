//
//  PlayerView.h
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/5/20.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlayerView : UIView
- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h;
- (void)setVideoSize:(GLuint)width height:(GLuint)height;
- (void)setViewFrame:(CGRect)frame;
- (void)close;
@end

NS_ASSUME_NONNULL_END
