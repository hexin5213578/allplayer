//
//  ConfigPanelView.h
//  AllcamApiDemo
//
//  Created by 秦骏 on 2021/11/24.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
typedef void (^LoginBlock)(NSString* accout,NSString*password,NSString*captcha);

typedef void (^ChangeCaptchaBlock)(void);

typedef void (^SaveConfigBlock)(NSString* ip,NSString*port);

@interface ConfigPanelView : UIView

@property (readwrite, nonatomic, copy) LoginBlock loginBlock;

@property (readwrite, nonatomic, copy) ChangeCaptchaBlock changeCaptchaBlock;

@property (readwrite, nonatomic, copy) SaveConfigBlock saveConfigBlock;

@property(nonatomic, strong)UIImageView* captchaImgV;

@property(nonatomic, strong) UITextField* textFieldAddress;

@property(nonatomic, strong) UITextField* textFieldPort;

@property(nonatomic, strong) UIButton* saveBtn;

@property (nonatomic, strong) UITextField* accoutTF;

@property (nonatomic, strong) UITextField* passwordTF;

@property (nonatomic, strong) UITextField* captchaTF;

@property(nonatomic, strong) UIButton* loginBtn;

@property(nonatomic, strong) UIButton* changeBtn;

-(void)updateCaptcha:(NSString*)captchaData;

+(ConfigPanelView*)popView:(ConfigPanelView*)pView;
@end

NS_ASSUME_NONNULL_END
