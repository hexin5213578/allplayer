//
//  VideoPanelView.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import "VideoPanelView.h"
#import "Masonry.h"
@interface VideoPanelView()
@property (nonatomic, strong) UIButton* fullBtn;//全屏 小屏
@property (nonatomic, strong) UIButton* multiBtn;//多屏 单屏

@property (nonatomic, strong) UIButton* sanpBtn;//抓拍
@property (nonatomic, strong) UIButton* videoBtn;//录像
@property (nonatomic, strong) UIButton* voiceBtn;//声音
@property (nonatomic, strong) UIButton* playBtn;//播放
@end
@implementation VideoPanelView
- (instancetype)initWithFrame:(CGRect)frame{
    if (self = [super initWithFrame:frame]) {
        [self initUI];
        self.fullBtn.selected = YES;
        self.videoBtn.selected = YES;
        self.voiceBtn.selected = YES;
    }
    return self;
}
-(void)initUI{
    [self addSubview:self.fullBtn];
    [self.fullBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.mas_right).offset(-5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];
    [self addSubview:self.multiBtn];
    [self.multiBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.fullBtn.mas_left).offset(-5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];

    [self addSubview:self.playBtn];
    [self.playBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.left.equalTo(self.mas_left).offset(5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];
    [self addSubview:self.sanpBtn];
    [self.sanpBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.multiBtn.mas_left).offset(-5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];
    [self addSubview:self.videoBtn];
    [self.videoBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.sanpBtn.mas_left).offset(-5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];
    [self addSubview:self.voiceBtn];
    [self.voiceBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.videoBtn.mas_left).offset(-5);
        make.height.mas_equalTo(40);
        make.width.mas_equalTo(60);
    }];
}

-(UIButton*)playBtn{
    if (!_playBtn) {
        _playBtn = [[UIButton alloc]init];
        [_playBtn setTitle:@"播放" forState:UIControlStateNormal];
        [_playBtn setTitle:@"暂停" forState:UIControlStateSelected ];
        [_playBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _playBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_playBtn addTarget:self action:@selector(playClick) forControlEvents:UIControlEventTouchUpInside];
        _playBtn.layer.borderColor = [UIColor whiteColor].CGColor;
        _playBtn.layer.borderWidth = 0.5;
        _playBtn.layer.cornerRadius = 5.0;
        _playBtn.layer.masksToBounds = YES;
    }
    return _playBtn;
}
-(UIButton*)fullBtn{
    if (!_fullBtn) {
        _fullBtn = [[UIButton alloc]init];
        [_fullBtn setTitle:@"全屏" forState:UIControlStateSelected];
        [_fullBtn setTitle:@"小屏" forState:UIControlStateNormal];
     
        [_fullBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _fullBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_fullBtn addTarget:self action:@selector(fullClick) forControlEvents:UIControlEventTouchUpInside];
        _fullBtn.layer.borderColor = [UIColor whiteColor].CGColor;
        _fullBtn.layer.borderWidth = 0.5;
        _fullBtn.layer.cornerRadius = 5.0;
        _fullBtn.layer.masksToBounds = YES;
    }
    return _fullBtn;
}
-(UIButton*)multiBtn{
    if (!_multiBtn) {
        _multiBtn = [[UIButton alloc]init];
        [_multiBtn setTitle:@"单屏" forState:UIControlStateNormal];
        //[_multiBtn setTitle:@"多屏" forState:UIControlStateSelected];
  
        [_multiBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _multiBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_multiBtn addTarget:self action:@selector(multiClick) forControlEvents:UIControlEventTouchUpInside];
        _multiBtn.layer.borderColor = [UIColor whiteColor].CGColor;
        _multiBtn.layer.borderWidth = 0.5;
        _multiBtn.layer.cornerRadius = 5.0;
        _multiBtn.layer.masksToBounds = YES;
    }
    return _multiBtn;
}
-(UIButton*)voiceBtn{
    if (!_voiceBtn) {
        _voiceBtn = [[UIButton alloc]init];
        [_voiceBtn setTitle:@"静音" forState:UIControlStateNormal];
        [_voiceBtn setTitle:@"声音" forState:UIControlStateSelected];
       
        [_voiceBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _voiceBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_voiceBtn addTarget:self action:@selector(voiceClick) forControlEvents:UIControlEventTouchUpInside];
        _voiceBtn.layer.borderColor = [UIColor whiteColor].CGColor;
        _voiceBtn.layer.borderWidth = 0.5;
        _voiceBtn.layer.cornerRadius = 5.0;
        _voiceBtn.layer.masksToBounds = YES;
    }
    return _voiceBtn;
}
-(UIButton*)sanpBtn{
    if (!_sanpBtn) {
        _sanpBtn = [[UIButton alloc]init];
        [_sanpBtn setTitle:@"抓拍" forState:UIControlStateNormal];
        [_sanpBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _sanpBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_sanpBtn addTarget:self action:@selector(sanpClick) forControlEvents:UIControlEventTouchUpInside];
        _sanpBtn.layer.borderColor = [UIColor whiteColor].CGColor;
        _sanpBtn.layer.borderWidth = 0.5;
        _sanpBtn.layer.cornerRadius = 5.0;
        _sanpBtn.layer.masksToBounds = YES;
    }
    return _sanpBtn;
}
-(UIButton*)videoBtn{
    if (!_videoBtn) {
        _videoBtn = [[UIButton alloc]init];
        [_videoBtn setTitle:@"停止" forState:UIControlStateNormal];
        [_videoBtn setTitle:@"录像" forState:UIControlStateSelected];
        [_videoBtn setTitleColor: [UIColor whiteColor] forState:UIControlStateNormal];
        _videoBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_videoBtn addTarget:self action:@selector(videoClick) forControlEvents:UIControlEventTouchUpInside];
        _videoBtn.layer.borderColor =[UIColor whiteColor].CGColor;
        _videoBtn.layer.borderWidth = 0.5;
        _videoBtn.layer.cornerRadius = 5.0;
        _videoBtn.layer.masksToBounds = YES;
    }
    return _videoBtn;
}
-(void)videoClick{
    
    if (self.actionBlock) {
        self.actionBlock(ActionType_video,self.videoBtn.isSelected);
    }
}
-(void)sanpClick{
    if (self.actionBlock) {
        self.actionBlock(ActionType_sanp,self.sanpBtn.isSelected);
    }
}
-(void)voiceClick{
    if (self.actionBlock) {
        self.actionBlock(ActionType_voice,self.voiceBtn.isSelected);
    }
}
-(void)action:(ActionType)actionType value:(BOOL)status{
    switch (actionType) {
        case ActionType_voice:
        {
            self.voiceBtn.selected = status;
        }
            break;
        case ActionType_video:
        {
            self.videoBtn.selected = status;
        }
            break;
            
        default:
            break;
    }
}
-(void)fullClick{
    if (self.fullBlock) {
        self.fullBlock(self.fullBtn.isSelected);
    }
}
-(void)multiClick{
    if (self.multiBlock) {
        self.multiBlock(self.multiBtn.isSelected);
    }
}
-(void)fullStatus:(BOOL)full{
    self.fullBtn.selected = full;
}
-(void)multiStatus:(BOOL)multi{
    self.multiBtn.selected = multi;
}

-(void)playClick{
    if (self.playBlock) {
        self.playBlock(self.playBtn.isSelected);
    }
}
-(void)playStatus:(BOOL)play{
    self.playBtn.selected = play;
}
@end
