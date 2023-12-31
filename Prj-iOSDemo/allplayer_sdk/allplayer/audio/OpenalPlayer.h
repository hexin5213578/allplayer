//
//  OpenalPlayer.h
//  FFmpeg-project
//
//  Created by huizai on 2017/10/26.
//  Copyright © 2017年 huizai. All rights reserved.
//

#import <Foundation/Foundation.h>
#import<Openal/Openal.h>

@interface OpenalPlayer : NSObject
@property(nonatomic,assign)int m_numprocessed;             //队列中已经播放过的数量
@property(nonatomic,assign) int m_numqueued;                //队列中缓冲队列数量
@property(nonatomic,assign) long long m_IsplayBufferSize;   //已经播放了多少个音频缓存数目
@property(nonatomic,assign) double m_oneframeduration;      //一帧音频数据持续时间(ms)
@property(nonatomic,assign) float m_volume;                 //当前音量volume取值范围(0~1)
@property(nonatomic,assign) int m_samplerate;               //采样率
@property(nonatomic,assign) int m_bit;                      //样本值
@property(nonatomic,assign) int m_channel;                  //声道数
@property(nonatomic,assign) int m_datasize;                 //一帧音频数据量
@property(nonatomic,assign) double playRate;                //播放速率

#pragma mark - 接口
+ (instancetype)sharedInstance;
-(int)initOpenAL;
-(int)updataQueueBuffer;
-(void)cleanUpOpenAL;
-(void)playSound;
-(void)stopSound;
-(int)openAudioFromQueue:(char*)data andWithDataSize:(int)dataSize andWithSampleRate:(int) aSampleRate andWithAbit:(int)aBit andWithAchannel:(int)aChannel;
@end
