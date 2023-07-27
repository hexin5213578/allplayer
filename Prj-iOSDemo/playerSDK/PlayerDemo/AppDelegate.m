//
//  AppDelegate.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import "AppDelegate.h"
#import "ViewController.h"
#import "AllCamPlayer.h"
#import "AFNetworking.h"

@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    ViewController * root = [[ViewController alloc] init];
    UINavigationController * rootNav = [[UINavigationController alloc] initWithRootViewController:root];
    self.window.rootViewController = rootNav;
    [self.window makeKeyAndVisible];
    //初始化播放器
    [AllCamPlayer initSDK];
    
    //监测网络状态
    [self AFNReachability];
    
    return YES;
}

-(void)AFNReachability
{
    //1.创建网络监听管理者
    AFNetworkReachabilityManager *manager = [AFNetworkReachabilityManager sharedManager];
    
    //2.监听网络状态的改变
    /*
     AFNetworkReachabilityStatusUnknown     = 未知
     AFNetworkReachabilityStatusNotReachable   = 没有网络
     AFNetworkReachabilityStatusReachableViaWWAN = 3G
     AFNetworkReachabilityStatusReachableViaWiFi = WIFI
     */
    [manager setReachabilityStatusChangeBlock:^(AFNetworkReachabilityStatus status) {
        switch (status) {
            case AFNetworkReachabilityStatusUnknown:
                NSLog(@"未知");
                break;
            case AFNetworkReachabilityStatusNotReachable:
                NSLog(@"没有网络");
                [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NETWORK"];
                //[MBProgressHUD showSuccess:NSLocalizedString(@"tip_nonet", nil)];
                break;
            case AFNetworkReachabilityStatusReachableViaWWAN:
                NSLog(@"4G");
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NETWORK"];
                //[MBProgressHUD showSuccess:NSLocalizedString(@"tip_4g", nil)];
                [[NSNotificationCenter defaultCenter] postNotificationName:@"netWorkChange" object:nil];
                break;
            case AFNetworkReachabilityStatusReachableViaWiFi:
                NSLog(@"WIFI");
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NETWORK"];
                //[MBProgressHUD showSuccess:NSLocalizedString(@"tip_wifi", nil)];
                [[NSNotificationCenter defaultCenter] postNotificationName:@"netWorkChange" object:nil];
                break;
                
            default:
                break;
        }
    }];
    //3.开始监听
    [[NSUserDefaults standardUserDefaults] synchronize];
    [manager startMonitoring];
}
#pragma mark - UISceneSession lifecycle


- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}


- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}


@end
