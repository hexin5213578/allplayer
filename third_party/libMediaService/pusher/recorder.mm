//
//  Recorder.m
//  RecordAndPlay
//
//  Created by yuweiyang on 2022/3/17.
//  Copyright © 2022 Topsky. All rights reserved.
//
#import "recorder.h"
#import <Openal/Openal.h>
#include "as_ring_cache.h"
#import "XBEchoCancellation.h"
#include <mutex>

@interface Recorder()
{
    int offset;
    as_ring_cache auidoCache;
    unsigned int dataLen;
    std::mutex mtx;
}
@end
@implementation Recorder

- (id)init
{
    self = [super init];
    if (self) {

        auidoCache.SetCacheSize(2048);

    }
    return self;
}

-(void)getAudioData: (unsigned char*)audioData andSize: (unsigned int*)size
{
    if(auidoCache.GetDataSize() >= 320)
    {
        auidoCache.Read((char*)audioData, 320);
        [self saveAudioData:[NSData dataWithBytes:audioData length:320] name:@"testsgssb4833.pcm" clearBefore:YES];
        *size = 320;
    }
}
-(int)cacheSize{
    int sizeCache = auidoCache.GetDataSize();
    return sizeCache;
}
-(void)getRealAudioData: (unsigned char*)audioData andSize: (unsigned int*)size
{
    if(auidoCache.GetDataSize() >= 640)
    {
        auidoCache.Read((char*)audioData, 640);
      //  [self saveAudioData:[NSData dataWithBytes:audioData length:640] name:@"testsgssb4833.pcm" clearBefore:YES];
        *size = 640;
    }
}


//开始录音
-(void)start:(int)sampeleRate :(int)mChannelsPerFrame :(int)mBitsPerChannel
{
    [self record];
}

//结束录音
-(void)stop
{
    [self stopRecord];
}

#pragma mark - 录音
- (void)record
{
    XBEchoCancellation *echo = [XBEchoCancellation shared];
    echo.bl_input = ^(AudioBufferList *bufferList) {
        AudioBuffer buffer = bufferList->mBuffers[0];
        //NSLog(@"%u",(unsigned int)buffer.mDataByteSize);
        NSData *pcmBlock = [NSData dataWithBytes:buffer.mData length:buffer.mDataByteSize];
        self->auidoCache.Write((const char*)buffer.mData,buffer.mDataByteSize);
        //NSLog(@"%u",(unsigned int)buffer.mDataByteSize);
        [self saveAudioData:pcmBlock name:@"testsgssb4822.pcm" clearBefore:YES];
    };
    [echo startInput];
    
}
-(void)realRecordBlock:(void (^)(AudioBuffer buffer)) done{
    XBEchoCancellation *echo = [XBEchoCancellation shared];
    echo.bl_input = ^(AudioBufferList *bufferList) {
        AudioBuffer buffer = bufferList->mBuffers[0];
        self->auidoCache.Write((const char*)buffer.mData,buffer.mDataByteSize);
        done(buffer);
    };
    [echo startInput];
}

- (void)stopRecord
{
    [[XBEchoCancellation shared] closeEchoCancellation];
    [[XBEchoCancellation shared] stop];
}

//保存录音文件
- (void)saveAudioData:(NSData *)data name:(NSString *)name clearBefore:(BOOL)clear{
    NSString *path=[[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject]stringByAppendingPathComponent:name];
    if (clear == YES) {
        if ( YES == [[NSFileManager defaultManager] fileExistsAtPath:path] ){
            [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
        }
    }else {
        if (NO == [[NSFileManager defaultManager] fileExistsAtPath:path]) {
            [[NSFileManager defaultManager] createFileAtPath:path contents:data attributes:nil];
        }else {
            NSFileHandle *fileHandle = [NSFileHandle fileHandleForUpdatingAtPath:path];
            [fileHandle seekToEndOfFile];  //将节点跳到文件的末尾
            [fileHandle writeData:data]; //追加写入数据
            [fileHandle closeFile];
        }
    }
}

@end

