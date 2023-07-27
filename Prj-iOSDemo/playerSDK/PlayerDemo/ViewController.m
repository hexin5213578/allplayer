//
//  ViewController.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import "ViewController.h"
#import "ConfigPanelView.h"
#import "HomePlayerView.h"
#import "Masonry.h"
#import "VideoPanelView.h"
#import "AllcamManagerSDK.h"
#import "DeviceListView.h"
#import "UIColor+Any.h"
#import "LocalViewController.h"
#import "AllCamPlayer.h"
#import "TestPartViewController.h"
#import "AllcamApi.h"

@interface ViewController ()

@property (nonatomic, strong) UIButton* livePlaybtn;
@property (nonatomic, strong) UIButton* videoPlaybtn;
@property (nonatomic, strong) UIButton* localSavebtn;

@property (nonatomic, strong) UIButton* settingBtn;//默认播放地址
@property (nonatomic, strong) UIButton* configBtn;//配置登录环境

@property (nonatomic, strong) UIView* cardView;
@property (nonatomic, strong) HomePlayerView* livePlayerView1;
@property (nonatomic, strong) HomePlayerView* livePlayerView2;
@property (nonatomic, strong) HomePlayerView* livePlayerView3;
@property (nonatomic, strong) HomePlayerView* livePlayerView4;

@property (nonatomic, strong) HomePlayerView* videoPlayerView;

@property (nonatomic, strong) ConfigPanelView* configPanelView;
@property (nonatomic, strong) DeviceListView* deviceListView;

@property (nonatomic, strong) AllCamPlayer* player;

@property (nonatomic, assign) NSInteger count;

@property (nonatomic, copy) NSDictionary *deciveDict;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.player = [[AllCamPlayer alloc] init];
    self.view.backgroundColor = [UIColor whiteColor];
    self.navigationItem.leftBarButtonItems = @[[[UIBarButtonItem alloc] initWithCustomView:self.configBtn],[[UIBarButtonItem alloc] initWithCustomView:self.settingBtn]];
    self.navigationItem.rightBarButtonItems = @[[[UIBarButtonItem alloc] initWithCustomView:self.localSavebtn],[[UIBarButtonItem alloc] initWithCustomView:self.videoPlaybtn],[[UIBarButtonItem alloc] initWithCustomView:self.livePlaybtn]];
    [AllcamManagerSDK initSDKWithConfigIp:defaultIp AndPort:defaultPort];
    [self initUI];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(netWorkChange) name:@"netWorkChange" object:nil];
    [self livePress];
}

- (void)netWorkChange {
    
}

-(UIButton*)livePlaybtn{
    if(!_livePlaybtn){
        _livePlaybtn = [[UIButton alloc]init];
        [_livePlaybtn setTitle:@"实时浏览" forState:UIControlStateNormal];
        _livePlaybtn.backgroundColor = [UIColor whiteColor];
        [_livePlaybtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        [_livePlaybtn setTitleColor: [UIColor themeColor] forState:UIControlStateSelected];
        _livePlaybtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_livePlaybtn addTarget:self action:@selector(livePress) forControlEvents:UIControlEventTouchUpInside];
    }
    return  _livePlaybtn;
}
-(UIButton*)videoPlaybtn{
    if(!_videoPlaybtn){
        _videoPlaybtn = [[UIButton alloc]init];
        [_videoPlaybtn setTitle:@"结束采集" forState:UIControlStateNormal];
        _videoPlaybtn.backgroundColor = [UIColor whiteColor];
        [_videoPlaybtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        [_videoPlaybtn setTitleColor: [UIColor themeColor] forState:UIControlStateSelected];
        _videoPlaybtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_videoPlaybtn addTarget:self action:@selector(videoPress) forControlEvents:UIControlEventTouchUpInside];
 
    }
    return  _videoPlaybtn;
}
-(UIButton*)localSavebtn{
    if(!_localSavebtn){
        _localSavebtn = [[UIButton alloc]init];
        [_localSavebtn setTitle:@"录音" forState:UIControlStateNormal];
        _localSavebtn.backgroundColor = [UIColor whiteColor];
        [_localSavebtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        _localSavebtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_localSavebtn addTarget:self action:@selector(savePress) forControlEvents:UIControlEventTouchUpInside];
 
    }
    return  _localSavebtn;
}
-(void)savePress{
    LocalViewController* localVC = [[LocalViewController alloc]init];
    [self.navigationController pushViewController:localVC animated:YES];
    
//    TestPartViewController* pvc = [[TestPartViewController alloc]init];
//    [self.navigationController pushViewController:pvc animated:YES];x5
    
//    NSArray *paramsArray =[self.player experienceInfoDataWithBussinessIdArr:@[@1]];
//    NSLog(@"%@",paramsArray);
//    NSLog(@"开始录音");
//    [AllcamApi audioRtspCameraId:[self.deciveDict objectForKey:@"deviceId"] agentType:@"1" Success:^(NSDictionary * _Nonnull result) {
//        [self.player audioStartWithRtspUrl:result[@"url"]];
//        } failure:^(NSDictionary * _Nonnull error) {
//            NSLog(@"1111");
//        }];
    
    
}


-(void)videoPress{
    NSLog(@"音频采集结束");
    //[self.player setVolume:70];
    [self.player audioStop];
}




-(UIButton*)settingBtn{
    if (!_settingBtn) {
        _settingBtn = [[UIButton alloc]init];
        [_settingBtn setTitle:@"登录环境" forState:UIControlStateSelected ];
        [_settingBtn setTitle:@"默认环境" forState:UIControlStateNormal];
        [_settingBtn setTitleColor: [UIColor themeColor] forState:UIControlStateSelected];
        [_settingBtn setTitleColor: [UIColor themeColor] forState:UIControlStateNormal];
        _settingBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_settingBtn addTarget:self action:@selector(settingClick) forControlEvents:UIControlEventTouchUpInside];
    }
    return _settingBtn;
}
-(UIButton*)configBtn{
    if (!_configBtn) {
        _configBtn = [[UIButton alloc]init];
        [_configBtn setTitle:@"配置" forState:UIControlStateNormal];
        _configBtn.backgroundColor = [UIColor whiteColor];
        [_configBtn setTitleColor: [UIColor themeColor] forState:UIControlStateNormal];
        _configBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_configBtn addTarget:self action:@selector(configPress) forControlEvents:UIControlEventTouchUpInside];
    }
    return _configBtn;
}
-(void)settingClick{
    self.settingBtn.selected = !self.settingBtn.selected;
    [self.livePlayerView1 settingConfig:self.settingBtn.isSelected];
    [self.videoPlayerView settingConfig:self.settingBtn.isSelected];
    if (self.settingBtn.isSelected) {
        self.title = [NSString stringWithFormat:@"rtsp://admin:Allcam2019@172.16.20.94/ch1/sub/av_stream"];
    }else{
        self.title = [NSString stringWithFormat:@"%@",[AllcamManagerSDK getIp]];
    }
}
-(void)configPress{
   
    self.configPanelView =  [ConfigPanelView popView:self.configPanelView];
    __weak typeof(self) weakself = self;
    self.configPanelView.loginBlock = ^(NSString * _Nonnull accout, NSString * _Nonnull password, NSString * _Nonnull captcha) {
        [weakself.deviceListView getDeviceList:@""];
    };
    
}
-(void)livePress{
    self.livePlaybtn.selected = YES;
    self.videoPlaybtn.selected = NO;
    self.livePlayerView1.hidden = NO;
    self.videoPlayerView.hidden = YES;
    [self.cardView mas_updateConstraints:^(MASConstraintMaker *make) {
        make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16);
    }];
}
//-(void)videoPress{
//    self.livePlaybtn.selected = NO;
//    self.videoPlaybtn.selected = YES;
//    self.livePlayerView.hidden = YES;
//    self.videoPlayerView.hidden = NO;
//    [self.cardView mas_updateConstraints:^(MASConstraintMaker *make) {
//        make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16 + 100 );;
//    }];
//    [self.deviceListView mas_updateConstraints:^(MASConstraintMaker *make) {
//        make.top.equalTo(self.cardView.mas_bottom);
//    }];
//
//    NSLog(@"音频采集结束");
//
//}
-(void)initUI{

    [self.view addSubview:self.cardView];
    [self.cardView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.view.mas_top).offset(SafeAreaTopHeight);
        make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16);
        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
    }];
    
    [self.cardView addSubview:self.livePlayerView1];
    [self.livePlayerView1 mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.cardView.mas_top);
        make.left.equalTo(self.cardView.mas_left).offset(0);
        make.height.mas_equalTo(([UIScreen mainScreen].bounds.size.width * 9/16));
        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
    }];

//    [self.cardView addSubview:self.livePlayerView2];
//    [self.livePlayerView2 mas_makeConstraints:^(MASConstraintMaker *make) {
//        make.top.equalTo(self.cardView.mas_top);
//        make.right.equalTo(self.cardView.mas_right).offset(0);
//        make.height.mas_equalTo(([UIScreen mainScreen].bounds.size.width * 9/16)/2);
//        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width/2);
//    }];
//
//    [self.cardView addSubview:self.livePlayerView3];
//    [self.livePlayerView3 mas_makeConstraints:^(MASConstraintMaker *make) {
//        make.top.equalTo(self.livePlayerView1.mas_bottom).offset(0);
//        make.left.equalTo(self.cardView.mas_left).offset(0);
//        make.height.mas_equalTo(([UIScreen mainScreen].bounds.size.width * 9/16)/2);
//        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width/2);
//    }];
//
//    [self.cardView addSubview:self.livePlayerView4];
//    [self.livePlayerView4 mas_makeConstraints:^(MASConstraintMaker *make) {
//        make.top.equalTo(self.livePlayerView2.mas_bottom).offset(0);
//        make.right.equalTo(self.cardView.mas_right).offset(0);
//        make.height.mas_equalTo(([UIScreen mainScreen].bounds.size.width * 9/16)/2);
//        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width/2);
//    }];
//
//    [self.cardView addSubview:self.videoPlayerView];
//    [self.videoPlayerView mas_makeConstraints:^(MASConstraintMaker *make) {
//        make.top.equalTo(self.self.cardView.mas_top);
//        make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16 + 100 );
//        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
//    }];

    [self.view addSubview:self.deviceListView];
    [self.deviceListView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.cardView.mas_bottom);
        make.bottom.equalTo(self.view.mas_bottom);
        make.left.equalTo(self.view.mas_left);
        make.right.equalTo(self.view.mas_right);
    }];
    
}
-(HomePlayerView*)livePlayerView1{
    if (!_livePlayerView1) {
        _livePlayerView1 = [[HomePlayerView alloc]initWithType:playerView_live];
        _livePlayerView1.backgroundColor = [UIColor blackColor];
        _livePlayerView1.parentVC = self;
    }
    return _livePlayerView1;
}
-(HomePlayerView*)livePlayerView2{
    if (!_livePlayerView2) {
        _livePlayerView2 = [[HomePlayerView alloc]initWithType:playerView_live];
        _livePlayerView2.backgroundColor = [UIColor blackColor];
        _livePlayerView2.parentVC = self;
    }
    return _livePlayerView2;
}
-(HomePlayerView*)livePlayerView3{
    if (!_livePlayerView3) {
        _livePlayerView3 = [[HomePlayerView alloc]initWithType:playerView_live];
        _livePlayerView3.backgroundColor = [UIColor blackColor];
        _livePlayerView3.parentVC = self;
    }
    return _livePlayerView3;
}
-(HomePlayerView*)livePlayerView4{
    if (!_livePlayerView4) {
        _livePlayerView4 = [[HomePlayerView alloc]initWithType:playerView_live];
        _livePlayerView4.backgroundColor = [UIColor blackColor];
        _livePlayerView4.parentVC = self;
    }
    return _livePlayerView4;
}
-(HomePlayerView*)videoPlayerView{
    if (!_videoPlayerView) {
        _videoPlayerView = [[HomePlayerView alloc]initWithType:playerView_local];
        _videoPlayerView.backgroundColor = [UIColor blackColor];
        _videoPlayerView.parentVC = self;
    }
    return _videoPlayerView;
}
-(DeviceListView*)deviceListView{
    if (!_deviceListView) {
        _deviceListView = [[DeviceListView alloc]init];
        __weak typeof(self) weakself = self;
        _deviceListView.selectedNodeBlock = ^(NSDictionary * _Nonnull nodeDic) {
            
            weakself.deciveDict = nodeDic;
            
            [weakself.livePlayerView1 selectDevice:nodeDic];
            
//            __strong typeof(self) stongself = weakself;
//            if (stongself.count == 0) {
//                stongself.count ++;
//                [stongself.livePlayerView1 selectDevice:nodeDic];
//                return;
//            }
            
//            if (stongself.count == 1) {
//                stongself.count ++;
//                [stongself.livePlayerView2 selectDevice:nodeDic];
//                return;
//            }
//
//            if (stongself.count == 2) {
//                stongself.count ++;
//                [stongself.livePlayerView3 selectDevice:nodeDic];
//                return;
//            }
//
//            if (stongself.count == 3) {
//                stongself.count = 0;
//                [stongself.livePlayerView4 selectDevice:nodeDic];
//                return;
//            }

//            if (weakself.videoPlaybtn.isSelected) {
//                [weakself.videoPlayerView selectDevice:nodeDic];
//            }
        };
    }
    return _deviceListView;
}
-(UIView*)cardView{
    if (!_cardView) {
        _cardView = [[UIView alloc]init];
    }
    return _cardView;
    
}
@end
