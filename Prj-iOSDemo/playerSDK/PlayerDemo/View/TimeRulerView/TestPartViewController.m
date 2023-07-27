//
//  TestPartViewController.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TestPartViewController.h"
#import "Masonry.h"
//#import "PlayerDemo-Bridging-Header.h"
#import "PlayerDemo-Swift.h"
@interface TestPartViewController ()
@property (nonatomic,strong)TimeRulerView* timeRulerView;

@property (nonatomic,strong)TimeRuler* timeRuler;
@end

@implementation TestPartViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    [self.view addSubview:self.timeRulerView];
    [self.timeRulerView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.view.mas_top).offset(200);
        make.left.equalTo(self.view.mas_left);
        make.right.equalTo(self.view.mas_right);
        make.height.mas_equalTo(100.0);
    }];
    [self.view addSubview:self.timeRuler];
    [self.timeRuler mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.timeRulerView.mas_bottom).offset(10);
        make.left.equalTo(self.view.mas_left);
        make.right.equalTo(self.view.mas_right);
        make.height.mas_equalTo(100.0);
    }];
}
-(TimeRulerView*)timeRulerView{
    if (!_timeRulerView) {
        _timeRulerView = [[TimeRulerView alloc]init];
    }
    return _timeRulerView;
}
-(TimeRuler*)timeRuler{
    if (!_timeRuler) {
        _timeRuler = [[TimeRuler alloc]initWithFrame:CGRectMake(0,0, self.view.frame.size.width,  100)];
    }
    return _timeRuler;
}


@end
