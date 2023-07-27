//
//  libAllplayer.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/5/11.
//

#import "libAllplayer.h"
#include "allplayer_manager.h"

@implementation libAllplayer
-(instancetype)init:(char*)path Loglevel:(int32_t)level{
    if (self = [super init]) {
        [self initplayer:path Loglevel:level];
    }
    return self;
}
-(int32_t)initplayer:(char*)path Loglevel:(int32_t)level{
    NSLog(@"初始化AllPlayer");
    int iRet = CAllplayerManager::GetInstance()->allplayerInit(path, level);
    if (iRet == -1) {
        NSLog(@"初始化AllPlayer失败");
    }
    return iRet;
   // return CAllplayerManager::GetInstance()->allplayerInit(path, level);
}
-(int32_t)playerExcute:(long)lBusiness Business:(BusinessInfoStruct)busInfo{
    return CAllplayerManager::GetInstance()->excuteBusiness(lBusiness, busInfo);
}

-(void)registerPlayerStatusCallback:(allplay_status_callback)pCallBack{
    CAllplayerManager::GetInstance()->registerStatusCallback(pCallBack);
}
-(void)registerPlayerDataCallback:(allplay_progress_callback)pCallBack{
    CAllplayerManager::GetInstance()->registerDataCallback(pCallBack);
}
@end
