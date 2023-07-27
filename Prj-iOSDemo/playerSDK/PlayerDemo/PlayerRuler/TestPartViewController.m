//
//  TestPartViewController.m
//  ChinaMobileProject
//
//  Created by ZC1AE6-4501-B15A on 2021/6/8.
//  Copyright © 2021 hello. All rights reserved.
//

#import "TestPartViewController.h"
#import "Masonry.h"

@interface TestPartViewController ()
@property (nonatomic,strong)TimeRulerView* timeRulerView;
@end

@implementation TestPartViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self.view addSubview:self.timeRulerView];
    [self.timeRulerView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.view.mas_top).offset(200);
        make.left.equalTo(self.view.mas_left);
        make.right.equalTo(self.view.mas_right);
        make.height.mas_equalTo(100.0);
    }];
    // Do any additional setup after loading the view.
}
-(TimeRulerView*)timeRulerView{
    if (!_timeRulerView) {
        _timeRulerView = [[TimeRulerView alloc]init];
    }
    return _timeRulerView;
}
/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
