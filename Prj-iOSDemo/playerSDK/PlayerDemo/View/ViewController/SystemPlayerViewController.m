//
//  SystemPlayerViewController.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/31.
//

#import "SystemPlayerViewController.h"
#import <AVKit/AVKit.h>
#import "UIColor+Any.h"
@interface SystemPlayerViewController ()
@property(nonatomic, strong)AVPlayer* player;
@property(nonatomic, strong)AVPlayerLayer* playerLayer;
@end

@implementation SystemPlayerViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    NSURL *url = [NSURL fileURLWithPath:self.mediaUrl];
    AVPlayerItem *item = [AVPlayerItem playerItemWithURL:url];
    self.player = [AVPlayer playerWithPlayerItem:item];
    [self.view.layer addSublayer:self.playerLayer];
    [self.player play];
}
-(AVPlayer *)player{
    if (!_player) {
        _player = [[AVPlayer alloc] init];
    }
    return _player;
}
#pragma mark - AVPlayerLayer
-(AVPlayerLayer *)playerLayer{
    if (!_playerLayer) {
        _playerLayer = [AVPlayerLayer playerLayerWithPlayer:self.player];
        _playerLayer.frame = CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height);
    }
    return _playerLayer;
}

@end
