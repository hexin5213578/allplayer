//
//  FullViewController.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/17.
//

#import "FullViewController.h"

@interface FullViewController ()

@end

@implementation FullViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor yellowColor];
    // Do any additional setup after loading the view.
    [self forceToOrientation:UIDeviceOrientationLandscapeLeft];
}
-(void)forceToOrientation:(UIDeviceOrientation)orientation{    
    NSNumber* orientationTarget = [NSNumber numberWithInt:(int)orientation];
    [[UIDevice currentDevice]setValue:orientationTarget forKey:@"orientation"];
}
-(void)viewWillDisappear:(BOOL)animated{
    [self forceToOrientation:UIDeviceOrientationPortrait];
}

@end
