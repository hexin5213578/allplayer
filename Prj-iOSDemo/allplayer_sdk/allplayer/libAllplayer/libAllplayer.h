//
//  libAllplayer.h
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/5/11.
//

#import <Foundation/Foundation.h>
#include "avs_player_common.h"
NS_ASSUME_NONNULL_BEGIN

typedef void (*allplay_status_callback)(long lBusiness, long busType, long status, const char* info);

typedef void (*allplay_progress_callback)(long lBusiness, long busType, long current, long total);

@interface libAllplayer : NSObject
-(instancetype)init:(char*)path Loglevel:(int32_t)level;
-(int32_t)initplayer:(char*)path Loglevel:(int32_t)level;

-(int32_t)playerExcute:(long)lBusiness Business:(BusinessInfoStruct)busInfo;

-(void)registerPlayerStatusCallback:(allplay_status_callback)pCallBack;
-(void)registerPlayerDataCallback:(allplay_progress_callback)pCallBack;
@end

NS_ASSUME_NONNULL_END
