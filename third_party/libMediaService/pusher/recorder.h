//
//  Recorder.h
//  RecordAndPlay
//
//  Created by yuweiyang on 2022/3/17.
//  Copyright © 2022 Topsky. All rights reserved.
//


#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#define RECORDER_NOTIFICATION_CALLBACK_NAME @"recorderNotificationCallBackName"
#define kNumberAudioQueueBuffers 3 //缓冲区设定3个
#define kDefaultSampleRate 8000    //采样率

@interface Recorder : NSObject

-(void)start:(int)sampeleRate :(int)mChannelsPerFrame :(int)mBitsPerChannel;

-(void)stop;

-(void)getAudioData: (unsigned char*)audioData andSize: (unsigned int*)size;

-(void)realRecordBlock:(void (^)(AudioBuffer buffer)) done;

-(void)getRealAudioData: (unsigned char*)audioData andSize: (unsigned int*)size;

-(int)cacheSize;
@end

