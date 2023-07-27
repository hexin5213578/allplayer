//
//  ViewController.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/18.
//

#import "ViewController.h"
#import "PlayerManager.h"
///#import "AFNetworking.h"
#import <WebKit/WebKit.h>
@interface ViewController ()
@property(nonatomic,strong)UIButton* playBtn;
@property(nonatomic,strong)WKWebView* webView;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
  //  [self checkNetStates];
    [self.view addSubview:self.playBtn];
    self.playBtn.center = self.view.center;
    
    PlayerManager* player = [PlayerManager sharedPlayerManager] ;
    [self.view addSubview:[player intPlayerView:self.view.frame.size.width]];
    
    
}
-(UIButton*)playBtn{
    if (!_playBtn) {
        _playBtn = [[UIButton alloc]initWithFrame:CGRectMake(0, 0, 60, 60)];
        _playBtn.backgroundColor = [UIColor redColor];
        [_playBtn setTitle:@"opengl" forState:UIControlStateNormal];
        [_playBtn addTarget:self action:@selector(videoSdlClick) forControlEvents:UIControlEventTouchUpInside];
    }
    return _playBtn;
}
-(void)videoSdlClick{
    //检测网络
    
    //初始化
    PlayerManager* player = [PlayerManager sharedPlayerManager] ;
   // [self.view addSubview:player.playerView1];
    [player preparePlayer:@""];
    
    

}
-(void)checkNetStates{
    [self.view addSubview:self.webView];
    NSURL * url = [NSURL URLWithString:@"http://www.baidu.com"];
   // NSURL * url = [NSURL URLWithString:@"rtsp://172.16.20.252/LiveMedia/ch1/Media1"];
    //rtsp://172.16.20.252/LiveMedia/ch1/Media1
    NSURLRequest * request = [NSURLRequest requestWithURL:url];
    [self.webView loadRequest:request];
}
- (WKWebView *)webView {
    if (!_webView) {
        _webView = [[WKWebView alloc] initWithFrame:CGRectMake(0,
                                                               20,
                                                               self.view.bounds.size.width,
                                                               self.view.bounds.size.height)];
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
