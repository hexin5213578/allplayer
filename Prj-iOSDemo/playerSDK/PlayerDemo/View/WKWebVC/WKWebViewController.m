//
//  WKWebViewController.m
//  WKWebViewDemo
//
//  Created by 秦骏 on 2021/11/15.
//
#import "WKWebViewController.h"

#import <WebKit/WebKit.h>
#import "WKWebView+JSInteraction.h"

@interface WKWebViewController ()<WKNavigationDelegate,WKUIDelegate,WKScriptMessageHandler>

@property (nonatomic,strong) WKWebView*  wkWebView;//webview

@property(nonatomic, strong) UIProgressView* progressView;//加载进度条

@end

@implementation WKWebViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self initData];
    [self initUIView];
    [self loadData];
 
}
-(void)initUIView{
    [self.view addSubview:self.wkWebView];
    self.wkWebView.frame = self.view.frame;
}
-(void)initData{
    //self.webUrl = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"html"];
  
}
-(void)loadData{
    NSURL *pathURL = [NSURL fileURLWithPath:self.webUrl];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:pathURL];
    [self.wkWebView loadRequest:request];
}
-(WKWebView*)wkWebView{
    if (!_wkWebView) {
        WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc]init];
        config.preferences = [[WKPreferences alloc]init];
        [config.userContentController addScriptMessageHandler:self name:@"getWKWebViewData"];
        _wkWebView  = [[WKWebView alloc] initWithFrame:self.view.bounds configuration:config];
        [_wkWebView addObserver:self forKeyPath:@"title" options:NSKeyValueObservingOptionNew context:NULL];
        [_wkWebView addObserver:self forKeyPath:@"estimatedProgress" options:NSKeyValueObservingOptionNew context:NULL];
        _wkWebView.navigationDelegate = self;
        _wkWebView.UIDelegate = self;
    }
    return _wkWebView;
}
-(UIProgressView*)progressView{
    if(!_progressView){
        _progressView = [[UIProgressView alloc]init];
        _progressView.tintColor = [UIColor blueColor];
    }
    return _progressView;
}
//返回交互  1、控件加载的网页内部返回 2、控件webview返回上层
-(void)goBack{
    if (self.wkWebView.canGoBack == YES) {
        [self.wkWebView goBack];
    }else{
        [self.navigationController popViewControllerAnimated:YES];
    }
}
//监听网页title显示网页标题
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {

    if ([keyPath isEqualToString:@"estimatedProgress"]) {
        if (object == self.wkWebView) {
            [self.progressView setAlpha:1.0f];
            [self.progressView setProgress:self.wkWebView.estimatedProgress animated:YES];
            if(self.wkWebView .estimatedProgress >= 1.0f) {
                [UIView animateWithDuration:0.3 delay:0.3 options:UIViewAnimationOptionCurveEaseOut animations:^{
                    [self.progressView setAlpha:0.0f];
                } completion:^(BOOL finished) {
                    [self.progressView setProgress:0.0f animated:NO];
                }];
            }
        }
        else
        {
            [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
        }
    }
    else if ([keyPath isEqualToString:@"title"])
    {
        if (object == self.wkWebView) {
            [self.navigationItem setTitle: self.wkWebView.title];
        }
        else
        {
            [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
        }
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}
//网页调用 getWKWebViewData 方法
- (void)userContentController:(nonnull WKUserContentController *)userContentController didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
    if ([message.name isEqualToString:@"getWKWebViewData"]) {
        NSString* parameterStr = message.body;//提取参数
        [self updateJSData:parameterStr];
    }
}
//WKWebView调用 updateJSData 方法 传参数据
-(void)updateJSData:(NSString*)parameterStr{
    NSMutableDictionary* dic = [[NSMutableDictionary alloc]init];
    //获取对应parameterStr 的值dataStr 保存成字典
    [dic setValue:@"dataStr" forKey:parameterStr];
    //字典转换成字符串
    NSString* dataStr =  [self.wkWebView dicToString:dic];
    //封装方法传参给JS
    NSString *js = [NSString stringWithFormat:@"updateJSData('%@')",dataStr];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.15 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.wkWebView evaluateJavaScript:js completionHandler:^(id _Nullable result, NSError * _Nullable error) {
            NSLog(@"error");
        }];
     });
}

@end
