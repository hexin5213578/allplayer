//
//  ViewController.h
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/18.
//

#import <UIKit/UIKit.h>
#import "PlayerView.h"

@interface ViewController : UIViewController

@property(nonatomic,strong) PlayerView * playerView;
@property(nonatomic,assign) CGFloat playerWidth;

@end

