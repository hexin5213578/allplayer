//
//  HomePlayerView.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import "HomePlayerView.h"
#import "VideoPanelView.h"
#import "Masonry.h"
#import "FullViewController.h"
#import "ViewController.h"
#import "UIColor+Any.h"
#import "PlayerViewCell.h"
#import "TimeRulerView.h"
@interface HomePlayerView()<UICollectionViewDataSource,UICollectionViewDelegate>

@property (nonatomic, strong) UICollectionView* multiPlayView;//多屏播放层

@property (nonatomic, strong) UICollectionViewFlowLayout *layout;

@property (nonatomic, strong) OpenGLView20* singPlayView;//单屏播放层
@property (nonatomic, strong) NSDictionary* selectPlayDic;

@property (nonatomic, strong) VideoPanelView* panelView;//播控层

@property (nonatomic, strong) FullViewController* fullViewController;//全屏层

@property (nonatomic, assign) NSInteger selectPlayIndex;//单屏播放序号

@property (nonatomic, strong) TimeRulerView* rulerView;

@property (nonatomic, assign) BOOL multiPlay;
@property (nonatomic, strong) NSMutableArray* multiPlayViewArr;
@property (nonatomic, strong) NSMutableArray* multiPlayArr;
@property (nonatomic, assign) BOOL defaultConfig;


@end
@implementation HomePlayerView
- (instancetype)initWithType:(PlayerViewType)playerviewType{
    if (self = [super init]) {
        self.playerviewType = playerviewType;
        self.selectPlayIndex = -1;
        self.siglePlayer = [[SiglePlayer alloc]init];
        self.multiPlay = NO;
        self.multiPlayArr = [[NSMutableArray alloc]init];
        self.multiPlayViewArr = [[NSMutableArray alloc]init];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationDidChange:) name:UIDeviceOrientationDidChangeNotification object:nil];
        [self initUI];
    }
    return self;
}

-(void)settingConfig:(BOOL)congfig{
    self.defaultConfig = congfig;
}
-(void)initUI{
    if (self.playerviewType == playerView_live ||self.playerviewType == playerView_local) {
        [self addSubview:self.multiPlayView];
        [self.multiPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.bottom.equalTo(self.mas_bottom);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.top.equalTo(self.mas_top);
        }];
        
        [self addSubview:self.singPlayView];
        [self.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.bottom.equalTo(self.multiPlayView.mas_bottom).offset(-10);
            make.left.equalTo(self.multiPlayView.mas_left).offset(10);
            make.right.equalTo(self.multiPlayView.mas_right).offset(-10);
            make.top.equalTo(self.multiPlayView.mas_top).offset(10);
        }];
        
        [self addSubview:self.panelView];
        [self.panelView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.bottom.equalTo(self.multiPlayView.mas_bottom);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.height.mas_equalTo(40);
        }];
    }else{
        [self addSubview:self.multiPlayView];
        [self.multiPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.bottom.equalTo(self.mas_bottom).offset( -100 );
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.top.equalTo(self.mas_top);
        }];
        
        [self addSubview:self.panelView];
        [self.panelView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.bottom.equalTo(self.multiPlayView.mas_bottom);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
            make.height.mas_equalTo(40);
        }];
        [self addSubview:self.rulerView];
        [self.rulerView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.panelView.mas_bottom);
            make.bottom.equalTo(self.mas_bottom);
            make.left.equalTo(self.mas_left);
            make.right.equalTo(self.mas_right);
        }];
    }
}
-(VideoPanelView*)panelView{
    if (!_panelView) {
        _panelView = [[VideoPanelView alloc]init];
        _panelView.backgroundColor = [UIColor maskColor];
        __weak typeof(self)weakself = self;
        _panelView.fullBlock = ^(BOOL full) {
            [weakself changeScreen:full];
        };
        _panelView.multiBlock = ^(BOOL multiplay) {
            [self.siglePlayer realStop];
            [self.siglePlayer loacalPlay];
//            if (weakself.multiPlay) {
//                if (weakself.selectPlayIndex > -1) {
//                    weakself.multiPlay = NO;
//                    [weakself.panelView multiStatus:YES];
//                    [weakself multiToSigleView:weakself.selectPlayIndex];
//                }else{
//                    [weakself.panelView multiStatus:YES];
//                    [weakself multiToSigleView:0];
//                }
//            }else{
//                weakself.multiPlay = YES;
//                [weakself.panelView multiStatus:NO];
//                [weakself sigleTomultiView:weakself.selectPlayIndex];
//            }
        };
        _panelView.playBlock = ^(BOOL play) {
            switch (weakself.playerviewType) {
                case playerView_local:
                {
                    if (weakself.multiPlay) {
                        
                    }else{
                        [weakself loacalPlay:play];
                    }
                  
                }
                    break;
                case playerView_video:
                {
                    
                }
                    break;
                case playerView_live:
                {
                    if (weakself.multiPlay) {
                        
                    }else{
                        [weakself livePlay:play];
                    }
                }
                    break;
                    
                default:
                    break;
            }
        
           
        };
        _panelView.actionBlock = ^(ActionType actionType, BOOL status) {
            [weakself dealWithAction:actionType status:status ];
        };
    }
    return _panelView;
}
-(void)livePlay:(BOOL)play{
    if (self.defaultConfig) {
        [self realDefaltPlay:play];
    }else{
        if (play) {
            [self.siglePlayer  realResume];
        }else{
            [self.siglePlayer  realStop];
        }
    }
}
-(void)dealWithAction:(ActionType)actionType  status:(BOOL)status{
    switch (actionType) {
        case ActionType_sanp:
        {
            [self snap];
        }
            break;
        case ActionType_video:
        {
            [self recordvideo:status];
        }
            break;
        case ActionType_voice:
        {
            [self voice:status];
        }
            break;
        default:
            break;
    }
}
-(void)voice:(BOOL)status{
    if (self.multiPlay) {
        
    }else{
        if (status) {
            [self.siglePlayer voiceSetting:70];
            [self.panelView action:ActionType_voice value:NO];
         
        }else{
            [self.siglePlayer voiceSetting:0];
            [self.panelView action:ActionType_voice value:YES];
        }
    }
}

-(void)recordvideo:(BOOL)status{
    if (self.multiPlay) {
        
    }else{
        if (status) {
            [self.siglePlayer videoRecord];
            [self.panelView action:ActionType_video value:NO];
        }else{
            [self.siglePlayer stopVideoRexord];
            [self.panelView action:ActionType_video value:YES];
        }
    }
}
-(void)snap{
    if (self.multiPlay) {
    }else{
        [self.siglePlayer snapShot];
    }
}
-(void)changeScreen:(BOOL)full{
    if (full) {
        [self switchToFullViewController];
    }else{
        [self backToNormalViewController];
    }
}


-(void)switchToFullViewController{
    [self.panelView fullStatus:NO];
    UIWindow *keyWindow = [[UIApplication sharedApplication] keyWindow];
    if (!keyWindow) {
        keyWindow = [[[UIApplication sharedApplication] windows] firstObject];
    }
    self.fullViewController = [[FullViewController alloc]init];
    [keyWindow addSubview: self.fullViewController.view];
    [ self.fullViewController.view addSubview:self];
    self.fullViewController.view.frame = CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width, [UIScreen mainScreen].bounds.size.height);
    [self mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(  self.fullViewController.view.mas_bottom);
        make.left.equalTo(  self.fullViewController.view.mas_left);
        make.right.equalTo(  self.fullViewController.view.mas_right);
        make.top.equalTo(  self.fullViewController.view.mas_top);
    }];
    [self.multiPlayView reloadData];
}
-(void)backToNormalViewController{
    [self.panelView fullStatus:YES];
    __weak typeof(self)weakself = self;
    [self.fullViewController dismissViewControllerAnimated:YES completion:^{
        [weakself.fullViewController.view removeFromSuperview];
        [weakself.parentVC.view addSubview:weakself];
        [weakself mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(weakself.parentVC.view.mas_top).offset(SafeAreaTopHeight);
            make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16);
            make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
        }];
        [weakself.multiPlayView reloadData];
    }];
}
-(void)orientationDidChange:(id)object{
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation == UIDeviceOrientationLandscapeLeft || orientation == UIDeviceOrientationLandscapeRight) {
        [self switchToFullViewController];
    }else{
        if ( self.fullViewController ) {
            [self backToNormalViewController];
        }
    }
}
//多屏选中推出小屏
-(void)multiToSigleView:(NSInteger)num{
    OpenGLView20* playView = [self.multiPlayViewArr objectAtIndex:num];
    SiglePlayer* player = [self.multiPlayArr objectAtIndex:num] ;

    self.singPlayView = playView;
    self.siglePlayer = player;
    
    [self insertSubview:self.singPlayView belowSubview:self.panelView];
    [self.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.multiPlayView.mas_bottom).offset(-10);
        make.left.equalTo(self.multiPlayView.mas_left).offset(10);
        make.right.equalTo(self.multiPlayView.mas_right).offset(-10);
        make.top.equalTo(self.multiPlayView.mas_top).offset(10);
    }];
}
//小屏归位到多屏
-(void)sigleTomultiView:(NSInteger)num{
    
    if (num == -1) {
        self.selectPlayIndex = 0;
        [self.multiPlayViewArr addObject:self.singPlayView];
        [self.multiPlayArr addObject:self.siglePlayer];
        [self.multiPlayView reloadData];
    }else{
        OpenGLView20* playView =  self.singPlayView ;
        SiglePlayer* player =  self.siglePlayer ;
        
        [self.multiPlayViewArr insertObject:playView atIndex:num];
        [self.multiPlayArr insertObject:player atIndex:num];
        [self.singPlayView removeFromSuperview];
        [self.multiPlayView reloadData];
        
    }
}
-(UICollectionView*)multiPlayView{
    if (!_multiPlayView) {
        _multiPlayView = [[UICollectionView alloc] initWithFrame:CGRectMake(0, 0, 0,0) collectionViewLayout:self.layout];
        _multiPlayView.backgroundColor = [UIColor clearColor];
        _multiPlayView.dataSource = self;
        _multiPlayView.delegate = self;
        _multiPlayView.bounces = NO;
        [_multiPlayView registerClass:[PlayerViewCell class] forCellWithReuseIdentifier:@"cell"];
    }
    return _multiPlayView;
}
- (UICollectionViewFlowLayout *)layout{
    if (!_layout) {
        _layout = [[UICollectionViewFlowLayout alloc]init];
    }
    return _layout;
}
- (nonnull __kindof UICollectionViewCell *)collectionView:(nonnull UICollectionView *)collectionView cellForItemAtIndexPath:(nonnull NSIndexPath *)indexPath {
    PlayerViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"cell" forIndexPath:indexPath];
    
    if (self.selectPlayIndex == indexPath.row) {
        cell.backgroundColor = [UIColor greenColor];
    }else{
        cell.backgroundColor = [UIColor clearColor];
    }
    NSArray *subviews = [[NSArray alloc] initWithArray:cell.contentView.subviews];
    for (UIView *subview in subviews) {
        [subview removeFromSuperview];
    }
    SiglePlayer* player = [self.multiPlayArr objectAtIndex:indexPath.row];
  
  //  OpenGLView20* view = [self.multiPlayViewArr objectAtIndex:indexPath.row];
    [cell view: [player getPlayerView] ];
    return cell;
}

- (NSInteger)collectionView:(nonnull UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return self.multiPlayViewArr.count;
}

- (CGSize) collectionView:(UICollectionView *)collectionView layout:(nonnull UICollectionViewLayout *)collectionViewLayout sizeForItemAtIndexPath:(nonnull NSIndexPath *)indexPath{
    return CGSizeMake(self.frame.size.width*0.5 - 5, self.frame.size.height*0.5 - 5);
}
- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath{
    self.selectPlayIndex = indexPath.row;
    [self.multiPlayView reloadData];
}

-(TimeRulerView*)rulerView{
    if (!_rulerView) {
        _rulerView = [[TimeRulerView alloc]init];
        _rulerView.delegate = self;
    }
    return _rulerView;
}
-(OpenGLView20*)singPlayView{
    if (!_singPlayView) {
        _singPlayView = [[OpenGLView20 alloc]init];
        _singPlayView.backgroundColor = [UIColor greenColor];
    }
    return _singPlayView;
}
-(void)selectDevice:(NSDictionary*)deviceDic{
    if (self.playerviewType == playerView_live) {
        if (self.multiPlay) {
            [self livemultiPlay:deviceDic];
        }else{
            [self.siglePlayer recordStop];
            [self livesiglePlay:deviceDic];
        }
    }else  if (self.playerviewType == playerView_video){
        if (self.multiPlay) {
            [self videomultiPlay:deviceDic];
        }else{
            [self.siglePlayer recordStop];
            [self videosiglePlay:deviceDic];
        }
    }
}
-(void)videomultiPlay:(NSDictionary*)deviceDic{
    if (self.multiPlayArr.count > 3) {
        SiglePlayer* player = [self.multiPlayArr objectAtIndex:self.selectPlayIndex];
        
        [player recordStop];
        __weak typeof(self) weakself = self;
        [player playerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
               dispatch_async(dispatch_get_main_queue(), ^{
                   [weakself.multiPlayViewArr insertObject:playew atIndex:self.selectPlayIndex];
                   [weakself.multiPlayView reloadData];
                   [player recordPlay];
               });
            }];
    }else{
        __weak typeof(self) weakself = self;
        SiglePlayer* player = [[SiglePlayer alloc]init];
        [weakself.multiPlayArr addObject:player];
        [player playerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
               dispatch_async(dispatch_get_main_queue(), ^{
                   [weakself.multiPlayViewArr addObject:playew];
                   [weakself.multiPlayView reloadData];
                   [player recordPlay];
               });
            }];
    }
}
-(void)videosiglePlay:(NSDictionary*)deviceDic{
    __weak typeof(self) weakself = self;
    [self.siglePlayer videoPlayerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, NSArray * _Nonnull videoArr, BOOL done) {
        dispatch_async(dispatch_get_main_queue(), ^{
            weakself.singPlayView = playew;
            [weakself insertSubview:weakself.singPlayView belowSubview:weakself.panelView];
            [weakself.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
                make.bottom.equalTo(weakself.multiPlayView.mas_bottom).offset(-10);
                make.left.equalTo(weakself.multiPlayView.mas_left).offset(10);
                make.right.equalTo(weakself.multiPlayView.mas_right).offset(-10);
                make.top.equalTo(weakself.multiPlayView.mas_top).offset(10);
            }];
            [weakself.rulerView initRulerViewWith:videoArr withPraModel:weakself.siglePlayer.primaryModel];
            weakself.selectPlayDic = deviceDic;
            [weakself.siglePlayer recordPlay];
            [weakself.panelView playStatus:YES];
            [weakself.panelView multiStatus:YES];
        });
    }];
}
-(void)dragTimeRulerPlayWithOffserTime:(CGFloat)playTime{
    [self.siglePlayer palyAtPoint:playTime WithSpeed:1.0];
}
-(void)dragTimeRulerBeign{
   
}

-(void)dragTimeRulerPlayWithOffserTime:(CGFloat)playTime withUrl:(NSString*)url{
    
}

-(void)playRecordWith:(NSString*)url cameraModel:(NSDictionary*)mode{
    
}

-(void)rulerToastTips:(NSString*)tips{
    
}

-(void)viewShowTips:(NSString*)tips{
    
}


-(void)livesiglePlay:(NSDictionary*)deviceDic{
    __weak typeof(self) weakself = self;
    [self.siglePlayer playerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
           dispatch_async(dispatch_get_main_queue(), ^{
               weakself.singPlayView = playew;
               [weakself insertSubview:weakself.singPlayView belowSubview:weakself.panelView];
               [weakself.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
                   make.bottom.equalTo(weakself.multiPlayView.mas_bottom).offset(-10);
                   make.left.equalTo(weakself.multiPlayView.mas_left).offset(10);
                   make.right.equalTo(weakself.multiPlayView.mas_right).offset(-10);
                   make.top.equalTo(weakself.multiPlayView.mas_top).offset(10);
               }];
           });
        weakself.selectPlayDic = deviceDic;
        [weakself.siglePlayer realPlay];
        [weakself.panelView playStatus:YES];
        [weakself.panelView multiStatus:YES];
        }];
}
-(void)livemultiPlay:(NSDictionary*)deviceDic{
    if (self.multiPlayArr.count > 3) {
        SiglePlayer* player = [self.multiPlayArr objectAtIndex:self.selectPlayIndex];
        
        [player realStop];
        __weak typeof(self) weakself = self;
        [player playerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
               dispatch_async(dispatch_get_main_queue(), ^{
                   [weakself.multiPlayViewArr insertObject:playew atIndex:self.selectPlayIndex];
                   [weakself.multiPlayView reloadData];
                   [player realPlay];
               });
            }];
    }else{
        __weak typeof(self) weakself = self;
        SiglePlayer* player = [[SiglePlayer alloc]init];
        [weakself.multiPlayArr addObject:player];
        [player playerData:deviceDic width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
               dispatch_async(dispatch_get_main_queue(), ^{
                   [weakself.multiPlayViewArr addObject:playew];
                   [weakself.multiPlayView reloadData];
                   [player realPlay];
               });
            }];
    }
}

-(void)realDefaltPlay:(BOOL)play{
    if (!play) {
        [self defaltPlay];
    }else{
        [self.siglePlayer realStop];
        [self.panelView playStatus:NO];
    }
}
-(void)defaltPlay{
    __weak typeof(self) weakself = self;
    [self.siglePlayer defaultPlaywidth:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
        dispatch_async(dispatch_get_main_queue(), ^{
            weakself.singPlayView = playew;
            [weakself insertSubview:weakself.singPlayView belowSubview:weakself.panelView];
            [weakself.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
                make.bottom.equalTo(weakself.multiPlayView.mas_bottom).offset(-10);
                make.left.equalTo(weakself.multiPlayView.mas_left).offset(10);
                make.right.equalTo(weakself.multiPlayView.mas_right).offset(-10);
                make.top.equalTo(weakself.multiPlayView.mas_top).offset(10);
            }];
            [weakself.siglePlayer realPlay];
            [weakself.panelView playStatus:YES];
            [weakself.panelView multiStatus:YES];
        });
    }];
}
-(void)selectVideo:(NSString*)path{
  
    if (self.multiPlay) {
    }else{
        [self loacalsiglePlay:path];
    }
}

-(void)loacalsiglePlay:(NSString*)path{
    __weak typeof(self) weakself = self;
    [self.siglePlayer loacalPlay:path width:self.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
        dispatch_async(dispatch_get_main_queue(), ^{
            weakself.singPlayView = playew;
            [weakself insertSubview:weakself.singPlayView belowSubview:weakself.panelView];
            [weakself.singPlayView mas_makeConstraints:^(MASConstraintMaker *make) {
                make.bottom.equalTo(weakself.multiPlayView.mas_bottom).offset(-10);
                make.left.equalTo(weakself.multiPlayView.mas_left).offset(10);
                make.right.equalTo(weakself.multiPlayView.mas_right).offset(-10);
                make.top.equalTo(weakself.multiPlayView.mas_top).offset(10);
            }];
            [weakself.siglePlayer loacalPlay];
            [weakself.panelView playStatus:YES];
            [weakself.panelView multiStatus:YES];
        });
    }];
}
-(void)loacalPlay:(BOOL)play{
    if(play){
        [self.siglePlayer localStop];
        [self.panelView playStatus:NO];
    }else{
        [self.siglePlayer loacalPlay];
        [self.panelView playStatus:YES];
    }
}
@end
