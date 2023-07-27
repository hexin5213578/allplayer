//
//  ViewController.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/18.
//

#import "ViewController.h"
#import "Masonry.h"
#import <WebKit/WebKit.h>
#import "AllPlayerSDK.h"

@interface ViewController ()
@property(nonatomic,strong) UIButton* playBtn;
@property(nonatomic,strong) UIButton* stopBtn;
@property(nonatomic,strong) UIButton* snapBtn;
@property(nonatomic,strong) UIButton* playForWardBtn;
@property(nonatomic,strong) UIButton* playBackWardBtn;
@property(nonatomic,strong) UIView* playerControlPanel;
@property(nonatomic,strong) WKWebView* webView;
@property(nonatomic,strong) AllPlayerSDK* aPlayer;
@property(nonatomic,strong) UIImageView* snapicV;
@property(nonatomic,strong) UIButton* upVolumeBtn;
@property(nonatomic,strong) UIButton* downVolumeBtn;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    
    // Do any additional setup after loading the view.
    [self checkNetStates];
    
    self.playerWidth = self.view.frame.size.width;
    //[self.view addSubview:self.playerView];
    [self.view addSubview:self.playBtn];
    [self.view addSubview:self.stopBtn];
    [self.view addSubview:self.snapBtn];
    [self.view addSubview:self.playForWardBtn];
    [self.view addSubview:self.playBackWardBtn];
    [self.view addSubview:self.snapicV];
    self.playBtn.frame =CGRectMake(self.view.frame.size.width/2 - 2*CGRectGetWidth(self.playBtn.frame), CGRectGetMaxY(self.playerView.frame) + 20, 60,40 );
    self.stopBtn.frame =CGRectMake(self.view.frame.size.width/2 - CGRectGetWidth(self.playBtn.frame)/2, CGRectGetMaxY(self.playerView.frame) + 20, 60,40 );
    self.snapBtn.frame =CGRectMake(self.view.frame.size.width/2 + CGRectGetWidth(self.playBtn.frame), CGRectGetMaxY(self.playerView.frame) + 20, 60,40 );
    self.playForWardBtn.frame =CGRectMake(self.view.frame.size.width/2 - 2*CGRectGetWidth(self.playBtn.frame), CGRectGetMaxY(self.playBtn.frame) + 20, 60,40 );
    self.playBackWardBtn.frame =CGRectMake(self.view.frame.size.width/2 + CGRectGetWidth(self.playBtn.frame), CGRectGetMaxY(self.snapBtn.frame) + 20, 60,40 );
    self.snapicV.frame = CGRectMake(0,CGRectGetMaxY(self.playForWardBtn.frame) + 20, self.playerWidth, self.playerWidth/16*9);
    self.aPlayer = [[AllPlayerSDK alloc]init];
    [self.aPlayer initAllPlayerSDk];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PlayerStatueChange:) name:kAllPlayerSdkPlayerStatueChangeNotification object:nil];
}
-(void)PlayerStatueChange:(NSNotification*)noti{
    NSLog(noti.description);
}
-(UIButton*)stopBtn{
    if (!_stopBtn) {
        _stopBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _stopBtn.backgroundColor = [UIColor redColor];
        [_stopBtn setTitle:@"暂停" forState:UIControlStateNormal];
        _stopBtn.hidden = YES;
        [_stopBtn addTarget:self action:@selector(stopPlay) forControlEvents:UIControlEventTouchUpInside];
    }
    return _stopBtn;
}
-(UIButton*)playBtn{
    if (!_playBtn) {
        _playBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _playBtn.backgroundColor = [UIColor redColor];
        [_playBtn setTitle:@"播放" forState:UIControlStateNormal];
        [_playBtn setTitle:@"停止" forState:UIControlStateSelected];
        _playBtn.selected = NO;
        [_playBtn addTarget:self action:@selector(videoClick) forControlEvents:UIControlEventTouchUpInside];
    }
    return _playBtn;
}
-(UIButton*)snapBtn{
    if (!_snapBtn) {
        _snapBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _snapBtn.backgroundColor = [UIColor redColor];
        [_snapBtn setTitle:@"抓拍" forState:UIControlStateNormal];
        [_snapBtn addTarget:self action:@selector(snapPlay) forControlEvents:UIControlEventTouchUpInside];
    }
    return _snapBtn;
}
-(UIImageView*)snapicV{
    if (!_snapicV) {
        _snapicV = [[UIImageView alloc]initWithFrame:CGRectMake(0, 0, 0, 0)];
        _snapicV.backgroundColor = [UIColor redColor];
        [_snapicV setContentMode:UIViewContentModeScaleAspectFit];
    }
    return _snapicV;;
}
-(UIView*)playerControlPanel{
    if (!_playerControlPanel) {
        _playerControlPanel = [[UIView alloc]init];
    }
    return _playerControlPanel;
}
-(void)videoClick{
    self.playBtn.selected = !self.playBtn.selected;
    if (self.playBtn.selected) {
        [self playPlayer];
    }else{
        [self stopPlay];
    }
}

-(UIButton*)playForWardBtn{
    if (!_playForWardBtn) {
        _playForWardBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _playForWardBtn.backgroundColor = [UIColor redColor];
        [_playForWardBtn setTitle:@"快进" forState:UIControlStateNormal];
        [_playForWardBtn addTarget:self action:@selector(playforward) forControlEvents:UIControlEventTouchUpInside];
    }
    return _playForWardBtn;
}
-(UIButton*)playBackWardBtn{
    if (!_playBackWardBtn) {
        _playBackWardBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _playBackWardBtn.backgroundColor = [UIColor redColor];
        [_playBackWardBtn setTitle:@"快退" forState:UIControlStateNormal];
        [_playBackWardBtn addTarget:self action:@selector(playbackward) forControlEvents:UIControlEventTouchUpInside];
    }
    return _playBackWardBtn;
}
-(UIButton*)upVolumeBtn{
    if (!_upVolumeBtn) {
        _upVolumeBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _upVolumeBtn.backgroundColor = [UIColor redColor];
        [_upVolumeBtn setTitle:@"音量+" forState:UIControlStateNormal];
        [_upVolumeBtn addTarget:self action:@selector(upvolume) forControlEvents:UIControlEventTouchUpInside];
    }
    return _upVolumeBtn;
}
-(UIButton*)downVolumeBtn{
    if (!_downVolumeBtn) {
        _downVolumeBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 30)];
        _downVolumeBtn.backgroundColor = [UIColor redColor];
        [_downVolumeBtn setTitle:@"音量+" forState:UIControlStateNormal];
        [_downVolumeBtn addTarget:self action:@selector(downvolume) forControlEvents:UIControlEventTouchUpInside];
    }
    return _downVolumeBtn;
}

-(void)playforward{
    float speed = [self.aPlayer getPlaySpeed];
    speed = speed + 1;
    [self.aPlayer palyAtPoint:0 WithSpeed:speed];
}
-(void)playbackward{
    float speed = [self.aPlayer getPlaySpeed];
    speed = speed / 2;
    [self.aPlayer palyAtPoint:0 WithSpeed:speed];
}
-(void)stopPlay{
    [self.aPlayer realTimeStop];
}
- (NSString *)makePathByFileName
{
    NSDate *nsdate = [NSDate date];
    NSDateFormatter * formatter2 = [[NSDateFormatter alloc] init];
    formatter2.dateFormat = @"yyyyMMddHHmmss";
    NSString * destDateString = [formatter2 stringFromDate:nsdate];
    NSString *fileName = [destDateString stringByAppendingString:@".jpg"];
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:fileName];
    return outputFileName;
}
-(void)snapPlay{
    NSString* path = [self makePathByFileName];
    [self.aPlayer snapPicture:path];
 // UIImage* imagepic =   [self.aPlayer snapPictureAtPath:path];
    
    UIImage *firstImg = [[UIImage alloc] initWithContentsOfFile:path];
    dispatch_async(dispatch_get_main_queue(),^{
        if (firstImg) {
            self.snapicV.image = firstImg;
        }
    });
  
}

-(void)getSnapPic{
    //UIImageView* snappic =【】"rtsp://172.16.20.252/LiveMedia/ch1/Media1""rtsp://218.204.223.237:554/live/1/66251FC11353191F/e7ooqwcfbqjoo80j.sdp"
}
-(void)playPlayer{
    self.playerView = [self.aPlayer getPlayerViewWithUrl:@"rtsp://172.16.20.252/LiveMedia/ch1/Media1" AndWidth:self.view.frame.size.width];
    [self.aPlayer realPlay];
    [self.view addSubview:self.playerView];
}
-(void)pausePlayer{
    [self.aPlayer realTimePause];
}
-(PlayerView*)playerView{
    if (!_playerView) {
        _playerView = [[PlayerView alloc] initWithFrame:CGRectMake(0, 44, self.playerWidth, self.playerWidth/16*9)];
        _playerView.backgroundColor = [UIColor redColor];
    }
    return _playerView;
}

-(void)checkNetStates{
    [self.view addSubview:self.webView];
    NSURL * url = [NSURL URLWithString:@"http://www.baidu.com"];
    NSURLRequest * request = [NSURLRequest requestWithURL:url];
    [self.webView loadRequest:request];
}
- (WKWebView *)webView {
    if (!_webView) {
        _webView = [[WKWebView alloc] initWithFrame:CGRectMake(0,
                                                              0,
                                                               0,
                                                               0)];
        _webView.backgroundColor = [UIColor whiteColor];
        _webView.scrollView.backgroundColor = [UIColor blackColor];
        _webView.opaque = NO;
        _webView.scrollView.showsHorizontalScrollIndicator = NO;
        _webView.scrollView.showsVerticalScrollIndicator = NO;
        _webView.opaque = NO;
        _webView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

    }
    return _webView;
}
@end
