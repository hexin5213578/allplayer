//
//  ConfigPanelView.m
//  AllcamApiDemo
//
//  Created by 秦骏 on 2021/11/24.
//

#import "ConfigPanelView.h"
#import "Masonry.h"
#import "UIColor+Any.h"
#import "AllcamManagerSDK.h"
@interface ConfigPanelView ()<UITextFieldDelegate>

@property(nonatomic,strong) UIView* cardView;

@property(nonatomic, strong) UITapGestureRecognizer *tapRecognizer;

@end
@implementation ConfigPanelView

+(ConfigPanelView*)popView:(ConfigPanelView*)pView{
    
    UIWindow *keyWindow = [[UIApplication sharedApplication] keyWindow];
    if (!keyWindow) {
        keyWindow = [[[UIApplication sharedApplication] windows] firstObject];
    }
    if (pView) {
        [keyWindow addSubview:pView];
        return pView;
    }else{
        ConfigPanelView *panelView = [[ConfigPanelView alloc]initWithFrame:CGRectMake(0, 0,[UIScreen mainScreen].bounds.size.width , [UIScreen mainScreen].bounds.size.height)];
        [keyWindow addSubview:panelView];
        return panelView;
    }
}
- (void)handletapPressInAlertViewGesture:(UITapGestureRecognizer *)sender
{
    if (sender.state == UIGestureRecognizerStateEnded){
        CGPoint location = [sender locationInView:nil];
        if (![self.cardView pointInside:[self.cardView convertPoint:location fromView:self.window] withEvent:nil]){
            [self dismissView];
        }
    }
}
-(void)dismissView{
    [self.window removeGestureRecognizer:self.tapRecognizer];
    [self removeFromSuperview];
}
-(instancetype)initWithFrame:(CGRect)frame{
    if (self = [super initWithFrame:frame]) {
        self.backgroundColor = [UIColor maskColor];
        self.tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handletapPressInAlertViewGesture:)];
        [self addGestureRecognizer:self.tapRecognizer];
        self.tapRecognizer.cancelsTouchesInView = NO;
        self.userInteractionEnabled = YES;
        [self initUI];
    }
    return self;
}
-(void)initUI{
    [self addSubview:self.cardView];
    [self.cardView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.mas_top).offset(SafeAreaTopHeight);
        make.left.equalTo(self.mas_left);
        make.height.mas_equalTo(300 + SafeAreaTopBottom);
        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
    }];
    
    [self.cardView addSubview:self.textFieldAddress];
    [self.textFieldAddress mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.cardView.mas_left).offset(30);
        make.height.offset(40);
        make.width.offset(160);
        make.top.equalTo(self.cardView.mas_top).offset(30);
    }];
    
    [self.cardView addSubview:self.textFieldPort];
    [self.textFieldPort mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.textFieldAddress.mas_right).offset(10);
        make.height.offset(40);
        make.width.offset(100);
        make.top.equalTo(self.cardView.mas_top).offset(30);
    }];
    [self.cardView addSubview:self.saveBtn];
    [self.saveBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.textFieldPort.mas_right).offset(10);
        make.height.offset(40);
        make.right.equalTo(self.cardView.mas_right).offset(-30);
        make.top.equalTo(self.cardView.mas_top).offset(30);
    }];
    [self.cardView addSubview:self.accoutTF];
    [self.accoutTF mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.cardView.mas_left).offset(30);
        make.height.offset(40);
        make.right.equalTo(self.cardView.mas_right).offset(-30);
        make.top.equalTo(self.textFieldAddress.mas_bottom).offset(30);
    }];
    [self.cardView addSubview:self.passwordTF];
    [self.passwordTF mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.cardView.mas_left).offset(30);
        make.height.offset(40);
        make.right.equalTo(self.cardView.mas_right).offset(-30);
        make.top.equalTo(self.accoutTF.mas_bottom).offset(10);
    }];
    [self.cardView addSubview:self.captchaTF];
    [self.captchaTF mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.cardView.mas_left).offset(30);
        make.right.equalTo(self.textFieldPort.mas_right);
        make.height.offset(40);
        make.top.equalTo(self.passwordTF.mas_bottom).offset(10);
    }];
    [self.cardView addSubview:self.changeBtn];
    [self.changeBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.captchaTF.mas_right).offset(10);
        make.height.offset(40);
        make.right.equalTo(self.cardView.mas_right).offset(-30);
        make.top.equalTo(self.passwordTF.mas_bottom).offset(10);
    }];
    
    [self.cardView addSubview:self.loginBtn];
    [self.loginBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.mas_left).offset(30);
        make.height.offset(40);
        make.right.equalTo(self.cardView.mas_right).offset(-30);
        make.top.equalTo(self.captchaTF.mas_bottom).offset(10);
    }];
    
}
-(UIView*)cardView{
    if (!_cardView) {
        _cardView = [[UIView alloc]init];
        _cardView.backgroundColor = [UIColor whiteColor];
    }
    return _cardView;
}
-(UITextField*)textFieldAddress{
    if (!_textFieldAddress) {
        _textFieldAddress  = [[UITextField alloc] init];
        [_textFieldAddress setFont:[UIFont systemFontOfSize:14]];
        _textFieldAddress.backgroundColor = UIColorFromHex(0XFAFAFA);
        _textFieldAddress.textColor =  UIColorFromHex(0X262626);
        _textFieldAddress.text = [AllcamManagerSDK getIp];
        [_textFieldAddress.layer setCornerRadius:(4.0)];
        [_textFieldAddress.layer setMasksToBounds:YES];
        [_textFieldAddress.layer setBorderWidth:( 0.5)];
        [_textFieldAddress.layer setBorderColor:[UIColorFromHex(0XCCCCCC) CGColor]];
        
        UILabel* labAddress = [[UILabel alloc]initWithFrame:CGRectMake(0, 0, 10, 0)];
        [labAddress setText:@" 地址: "];
        [labAddress setFont:[UIFont systemFontOfSize:15.0]];
        [labAddress setTextAlignment:NSTextAlignmentLeft];
        [labAddress setTextColor:UIColorFromHex(0X666666)];
        
        _textFieldAddress.leftView = labAddress;
        _textFieldAddress.leftViewMode = UITextFieldViewModeAlways;
        _textFieldAddress.delegate = self;
    }
    return _textFieldAddress;
}
-(UIButton*)saveBtn{
    if(!_saveBtn){
        _saveBtn = [[UIButton alloc]init];
        [_saveBtn setTitle:@"保存" forState:UIControlStateNormal];
        _saveBtn.backgroundColor = [UIColor whiteColor];
        [_saveBtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        _saveBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_saveBtn addTarget:self action:@selector(saveClick) forControlEvents:UIControlEventTouchUpInside];
        _saveBtn.layer.cornerRadius = 5.0;
        _saveBtn.layer.masksToBounds = YES;
        _saveBtn.layer.borderColor = UIColorFromHex(0XCCCCCC).CGColor;
        _saveBtn.layer.borderWidth = 0.5;
    }
    return  _saveBtn;
}
-(void)saveClick{
    NSString* configIp = self.textFieldAddress.text;
    NSString* Port = self.textFieldPort.text;
    NSString *http;
    if ([Port isEqualToString:@"10000"]) {
        http = @"http";
    }else if([Port isEqualToString:@"10002"]){
        http = @"https";
    }
    NSString *domain = [NSString stringWithFormat:@"%@://%@:%@",http,configIp,Port];
    
    if (![configIp isEqualToString:@""] && ![Port isEqualToString:@""]) {
        [AllcamManagerSDK updateSDKWithConfigSite:domain];
    }
}
-(UIButton*)changeBtn{
    if(!_changeBtn){
        _changeBtn = [[UIButton alloc]init];
        [_changeBtn setTitle:@"换一张" forState:UIControlStateNormal];
        _changeBtn.backgroundColor = [UIColor whiteColor];
        [_changeBtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        _changeBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_changeBtn addTarget:self action:@selector(getCaptchaData) forControlEvents:UIControlEventTouchUpInside];
        _changeBtn.layer.cornerRadius = 5.0;
        _changeBtn.layer.masksToBounds = YES;
        _changeBtn.layer.borderColor = UIColorFromHex(0XCCCCCC).CGColor;
        _changeBtn.layer.borderWidth = 0.5;
    }
    return  _changeBtn;
}

-(void)getCaptchaData{
    __weak typeof(self) weakself = self;
    [AllcamManagerSDK getCodeSuccess:^(NSDictionary * _Nonnull result) {
        [weakself updateCaptcha:result[@"captchaData"]];
    } failure:^(NSDictionary * _Nonnull error) {
      
    }];
}

-(UITextField*)textFieldPort{
    if (!_textFieldPort) {
        _textFieldPort  = [[UITextField alloc] init];
        [_textFieldPort setFont:[UIFont systemFontOfSize:14]];
        _textFieldPort.backgroundColor = UIColorFromHex(0XFAFAFA);
        _textFieldPort.textColor =  UIColorFromHex(0X262626);
        _textFieldPort.text = [AllcamManagerSDK getPort];
        [_textFieldPort.layer setCornerRadius:(4.0)];
        [_textFieldPort.layer setMasksToBounds:YES];
        [_textFieldPort.layer setBorderWidth:( 0.5)];
        [_textFieldPort.layer setBorderColor:[UIColorFromHex(0XCCCCCC) CGColor]];
        
        UILabel* labPort = [[UILabel alloc]initWithFrame:CGRectMake(0, 0, 10, 0)];
        [labPort setText:@" 端口: "];
        [labPort setFont:[UIFont systemFontOfSize:15.0]];
        [labPort setTextAlignment:NSTextAlignmentLeft];
        [labPort setTextColor:UIColorFromHex(0X666666)];
        
        _textFieldPort.leftView = labPort;
        _textFieldPort.leftViewMode = UITextFieldViewModeAlways;
        _textFieldPort.delegate = self;
    }
    return _textFieldPort;
}
-(UITextField*)passwordTF{
    if (!_passwordTF) {
        _passwordTF = [[UITextField alloc] init];
        [_passwordTF setBackgroundColor:[UIColor whiteColor]];
        [_passwordTF setFont:[UIFont systemFontOfSize:13.0]];
        [_passwordTF.layer setCornerRadius:(4.0)];
        [_passwordTF.layer setMasksToBounds:YES];
        [_passwordTF.layer setBorderWidth:( 0.5)];
        [_passwordTF.layer setBorderColor:[UIColorFromHex(0XCCCCCC) CGColor]];
        UILabel* labPassword = [[UILabel alloc]initWithFrame:CGRectMake(0, 0, 10, 0)];
        [labPassword setText:@" 密码: "];
        [labPassword setFont:[UIFont systemFontOfSize:15.0]];
        [labPassword setTextAlignment:NSTextAlignmentLeft];
        [labPassword setTextColor:UIColorFromHex(0X666666)];
        _passwordTF.leftView = labPassword;
        _passwordTF.leftViewMode = UITextFieldViewModeAlways;
        _passwordTF.textColor = UIColorFromHex(0X1A1A1A);
        _passwordTF.text = @"123456";//@"smp!@2019";//@"Aqtk@2022"; //@"123456";
        _passwordTF.delegate = self;
    }
    return _passwordTF;
}
-(UITextField*)accoutTF{
    if (!_accoutTF) {
        _accoutTF = [[UITextField alloc] init];
        [_accoutTF setBackgroundColor:[UIColor whiteColor]];
        [_accoutTF setFont:[UIFont systemFontOfSize:13.0]];
        [_accoutTF.layer setCornerRadius:(4.0)];
        [_accoutTF.layer setMasksToBounds:YES];
        [_accoutTF.layer setBorderWidth:( 0.5)];
        [_accoutTF.layer setBorderColor:[UIColorFromHex(0XCCCCCC) CGColor]];
        UILabel* labAccout = [[UILabel alloc]initWithFrame:CGRectMake(0, 0, 10, 0)];
        [labAccout setText:@" 账号: "];
        [labAccout setFont:[UIFont systemFontOfSize:15.0]];
        [labAccout setTextAlignment:NSTextAlignmentLeft];
        [labAccout setTextColor:UIColorFromHex(0X666666)];
        _accoutTF.leftView = labAccout;
        _accoutTF.leftViewMode = UITextFieldViewModeAlways;
        _accoutTF.textColor = UIColorFromHex(0X1A1A1A);
        _accoutTF.text = @"admin";//@"aqtkqly";//
        _accoutTF.delegate = self;
    }
    return _accoutTF;
}
-(UITextField*)captchaTF{
    if (!_captchaTF) {
        _captchaTF = [[UITextField alloc] init];
        [_captchaTF setBackgroundColor:[UIColor whiteColor]];
        [_captchaTF setFont:[UIFont systemFontOfSize:13.0]];
        [_captchaTF.layer setCornerRadius:(4.0)];
        [_captchaTF.layer setMasksToBounds:YES];
        [_captchaTF.layer setBorderWidth:( 0.5)];
        [_captchaTF.layer setBorderColor:[UIColorFromHex(0XCCCCCC) CGColor]];
        UILabel* labAccout = [[UILabel alloc]initWithFrame:CGRectMake(0, 0, 10, 0)];
        [labAccout setText:@" 验证码: "];
        [labAccout setFont:[UIFont systemFontOfSize:15.0]];
        [labAccout setTextAlignment:NSTextAlignmentLeft];
        [labAccout setTextColor:UIColorFromHex(0X666666)];
        _captchaTF.leftView = labAccout;
        _captchaTF.leftViewMode = UITextFieldViewModeAlways;
        _captchaTF.rightView = [self makeRightView];
        _captchaTF.rightViewMode = UITextFieldViewModeAlways;
        _captchaTF.textColor = UIColorFromHex(0X1A1A1A);
        _captchaTF.delegate = self;
    }
    return _captchaTF;
}
-(UIView*)makeRightView{
    UIView *view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 80, 40)];
    [view addSubview:self.captchaImgV];
    return view;
}
-(UIImageView*)captchaImgV{
    if (!_captchaImgV) {
        _captchaImgV = [[UIImageView alloc]initWithFrame:CGRectMake(0, 0, 80, 40)];
        _captchaImgV.contentMode = UIViewContentModeScaleAspectFit;
    }
    return _captchaImgV;
}
-(UIButton*)loginBtn{
    if(!_loginBtn){
        _loginBtn = [[UIButton alloc]init];
        [_loginBtn setTitle:@"登录" forState:UIControlStateNormal];
        _loginBtn.backgroundColor = [UIColor whiteColor];
        [_loginBtn setTitleColor: UIColorFromHex(0X666666) forState:UIControlStateNormal];
        _loginBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_loginBtn addTarget:self action:@selector(loginClick) forControlEvents:UIControlEventTouchUpInside];
        _loginBtn.layer.cornerRadius = 5.0;
        _loginBtn.layer.masksToBounds = YES;
        _loginBtn.layer.borderColor = UIColorFromHex(0XCCCCCC).CGColor;
        _loginBtn.layer.borderWidth = 0.5;
    }
    return  _loginBtn;
}
-(void)loginClick{
    NSString* acount =  self.accoutTF.text;
    NSString* password =  self.passwordTF.text;
    NSString* captcha =  self.captchaTF.text;
    __weak typeof(self) weakself = self;
    [AllcamManagerSDK loginWithAcount:acount AndPassword:password Code:captcha Success:^(NSDictionary * _Nonnull result) {
        if (weakself.loginBlock) {
            weakself.loginBlock(acount,password,captcha);
            [weakself dismissView];
        }

    } failure:^(NSDictionary * _Nonnull error) {
  
    }];
 
}
-(void)updateCaptcha:(NSString*)captchaData{
    NSURL *baseImageUrl = [NSURL URLWithString:captchaData];
    NSData *imageData = [NSData dataWithContentsOfURL:baseImageUrl];
    self.captchaImgV.image = [[UIImage alloc]initWithData:imageData];
}
- (BOOL)textFieldShouldReturn:(UITextField *)textField{
    return [textField resignFirstResponder];
}

@end
