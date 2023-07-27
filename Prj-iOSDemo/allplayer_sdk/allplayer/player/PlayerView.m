//
//  PlayerView.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/5/20.
//

#import "PlayerView.h"
#import "OpenGLView20.h"
@interface PlayerView()
@property(nonatomic,strong) OpenGLView20* openglView;
@end
@implementation PlayerView
-(instancetype)initWithFrame:(CGRect)frame{
    self = [super initWithFrame:frame];
    if(self){
        [self addSubview:self.openglView];
    }
    return self;
}
-(OpenGLView20*)openglView{
    if (!_openglView) {
        _openglView = [[OpenGLView20 alloc] initWithFrame:CGRectMake(0, 0,self.frame.size.width,self.frame.size.height )];
        
    }
    return _openglView;
}
- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h{
    [self.openglView displayYUV420pData:data width:w height:h];
}
- (void)setVideoSize:(GLuint)width height:(GLuint)height{
    [self.openglView setVideoSize:width height:height];
}

-(void)setViewFrame:(CGRect)frame{
    self.frame = frame;
    self.openglView.frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
}
- (void)close{
    [self.openglView clearFrame];
}
@end
