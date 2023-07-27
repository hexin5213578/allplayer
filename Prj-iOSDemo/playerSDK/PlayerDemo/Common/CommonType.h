//
//  CommonType.h
//  SmartShop
//
//  Created by 秦骏 on 2021/12/10.
//

#ifndef CommonType_h
#define CommonType_h

typedef NS_ENUM(NSInteger , PlayerViewType) {
    playerView_live = 0,
    playerView_video,
    playerView_local,
};
typedef NS_ENUM(NSInteger , ActionType) {
    ActionType_sanp = 0,//抓拍
    ActionType_video,//录像
    ActionType_voice,//声音
};


#endif /* CommonType_h */
