//
//  OpenGLView.m
//  MyTest
//
//  Created by smy on 12/20/11.
//  Copyright (c) 2011 ZY.SYM. All rights reserved.
//

#import "openGL20.h"
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>
#include <sys/time.h>

//#define PRINT_CALL 1

@interface OpenGLView20()
{
    /**
     OpenGL绘图上下文
     */
    EAGLContext             *_glContext;
    
    /**
     帧缓冲区
     */
    GLuint                  _framebuffer;
    
    /**
     渲染缓冲区
     */
    GLuint                  _renderBuffer;
    
    GLsizei                 _viewScale;
    
    CGSize                  _viewSize;
    
    GLint                   _viewId;
    
    BOOL                    _openglInited;
    
#ifdef DEBUG
    struct timeval      _time;
    NSInteger           _frameRate;
#endif
}

/**
 视频宽高
 */
@property(nonatomic) GLuint videoW;

@property(nonatomic) GLuint videoH;

/*
 创建缓冲区
 @return 成功返回TRUE 失败返回FALSE
 */
- (BOOL)createFrameAndRenderBuffer;

/**
 销毁缓冲区
 */
- (void)destoryFrameAndRenderBuffer;

@end

@implementation OpenGLView20

- (BOOL)doInit
{
    CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
    //eaglLayer.opaque = YES;
    
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat,
                                    //[NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
                                    nil];
    self.contentScaleFactor = [UIScreen mainScreen].scale;
    _viewScale = [UIScreen mainScreen].scale;
    _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    return true;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        if (![self doInit])
        {
            self = nil;
        }
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        if (![self doInit])
        {
            self = nil;
        }
    }
    return self;
}

- (void)layoutSubviews
{
   // dispatch_async(dispatch_get_global_queue(0, 0), ^{
    if(!_openglInited)
    {
        @synchronized(self)
        {
            [EAGLContext setCurrentContext:_glContext];
            [self destoryFrameAndRenderBuffer];
            [self createFrameAndRenderBuffer];
            auto scale = [UIScreen mainScreen].scale;
            auto bound = self.bounds.size;
            glViewport(1, 1, self.bounds.size.width*_viewScale - 2, self.bounds.size.height*_viewScale - 2);
        }
        _openglInited = true;
    }
    _viewSize = self.bounds.size;
    _viewId = self.tag;
  //  });
}

#pragma mark - 设置openGL
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (BOOL)createFrameAndRenderBuffer
{
    glGenFramebuffers(1, &_framebuffer);
    glGenRenderbuffers(1, &_renderBuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    
    if (![_glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer])
    {
        NSLog(@"attach渲染缓冲区失败");
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        NSLog(@"创建缓冲区错误 0x%x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return NO;
    }
    return YES;
}
		
- (void)destoryFrameAndRenderBuffer
{
    if (_framebuffer)
    {
        glDeleteFramebuffers(1, &_framebuffer);
    }
    
    if (_renderBuffer)
    {
        glDeleteRenderbuffers(1, &_renderBuffer);
    }
    
    _framebuffer = 0;
    _renderBuffer = 0;
}

#pragma mark - 接口

- (bool)Init
{
    if(!_glContext || ![EAGLContext setCurrentContext:_glContext])
    {
        return false;
    }
    return true;
}

- (void)Draw
{
    //@synchronized(self)
    {
        BOOL isSucess = [EAGLContext setCurrentContext:_glContext];
        //changeSurface的作用？
        //glViewport(1, 1, _viewSize.width*_viewScale-2, _viewSize.height*_viewScale-2);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
        [_glContext presentRenderbuffer:GL_RENDERBUFFER];
    }
    
#ifdef DEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("GL_ERROR=======>%d\n", err);
    }
    struct timeval nowtime;
    gettimeofday(&nowtime, NULL);
    if (nowtime.tv_sec != _time.tv_sec)
    {
        printf("视频 %d 帧率:   %d\n", _viewId, _frameRate);
        memcpy(&_time, &nowtime, sizeof(struct timeval));
        _frameRate = 0;
    }
    else
    {
        _frameRate++;
    }
#endif
}

- (void)ClearFrame
{
    //if ([self window])
    {
        [EAGLContext setCurrentContext:_glContext];
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
        [_glContext presentRenderbuffer:GL_RENDERBUFFER];
    }
}

- (void) Close
{
    struct timeval nowtime;
    gettimeofday(&nowtime, NULL);
    printf("Close opGL20 ClearFrame() BEGIN %ld \n",nowtime.tv_sec);
    [self ClearFrame];
    if(_glContext) {
        _glContext = nil;
    }
    gettimeofday(&nowtime, NULL);
    printf("Close opGL20 ClearFrame() END %ld \n",nowtime.tv_sec);
    [self destoryFrameAndRenderBuffer];
    gettimeofday(&nowtime, NULL);
    printf("Close opGL20 destoryFrameAndRenderBuffer() %ld \n",nowtime.tv_sec);
}

@end
