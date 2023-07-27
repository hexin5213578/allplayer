//
//  ViewController.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/18.
//

#import "ViewController.h"
#import "PlayerManager.h"
@interface ViewController ()
@property(nonatomic,strong)UIButton* playBtn;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    [self.view addSubview:self.playBtn];
    self.playBtn.center = self.view.center;
    
    
}
-(UIButton*)playBtn{
    if (!_playBtn) {
        _playBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 60)];
        _playBtn.backgroundColor = [UIColor redColor];
        [_playBtn setTitle:@"sdl" forState:UIControlStateNormal];
        [_playBtn addTarget:self action:@selector(videoSdlClick) forControlEvents:UIControlEventTouchUpInside];
    }
    return _playBtn;
}
-(void)videoSdlClick{
    //初始化
    PlayerManager* player = [PlayerManager sharedPlayerManager] ;
    [player preparePlayer:@"" width:self.view.frame.size.width];

}

@end
