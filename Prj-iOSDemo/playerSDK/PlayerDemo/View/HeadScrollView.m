//
//  HeadScrollView.m
//  AllcamApiDemo
//
//  Created by 秦骏 on 2021/12/1.
//

#import "HeadScrollView.h"
#import "Masonry.h"

@implementation HeadScrollView
-(instancetype)initWithFrame:(CGRect)frame{
    if (self = [super initWithFrame:frame]) {
        self.showsVerticalScrollIndicator = NO;
        self.showsHorizontalScrollIndicator = NO;
        self.headNodeArr = [[NSMutableArray alloc]init];
        self.headNodeViewArr = [[NSMutableArray alloc]init];
        [self initUI];
    }
    return self;
}
-(void)initUI{
    [self addSubview:self.deviceBtn];
    [self.deviceBtn mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.mas_left).offset(10);
        make.right.equalTo(self.mas_right).offset(-10);
        make.height.offset(30);
        make.centerX.equalTo(self.mas_centerX);
        make.top.equalTo(self.mas_top).offset(5);
    }];
}
-(UIButton*)deviceBtn{
    if (!_deviceBtn) {
        _deviceBtn = [[UIButton alloc]init];
        [_deviceBtn setTitle:@"获取设备列表" forState:UIControlStateNormal];
        [_deviceBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
        [_deviceBtn addTarget:self action:@selector(deviceClick) forControlEvents:UIControlEventTouchUpInside];
        _deviceBtn.layer.cornerRadius = 5.0;
        _deviceBtn.layer.masksToBounds = YES;
        _deviceBtn.layer.borderColor = UIColorFromHex(0XCCCCCC).CGColor;
        _deviceBtn.layer.borderWidth = 0.5;
    }
    return _deviceBtn;
}
-(void)deviceClick{
    if (self.getDeviceBlock) {
        self.getDeviceBlock();
    }
}
-(void)addhHead:(NSDictionary*)nodeDic{
    self.deviceBtn.hidden = YES;
    [self.headNodeArr addObject:nodeDic];
    NSInteger index = [self.headNodeArr indexOfObject:nodeDic];
    NSString* title = [NSString stringWithFormat:@"%@ >",[nodeDic objectForKey:@"label"]];
    NSDictionary *attrs = @{NSFontAttributeName : [UIFont systemFontOfSize:16]};
    CGSize size=[title sizeWithAttributes:attrs];
    UIButton* headBtn = [[UIButton alloc]initWithFrame:CGRectMake([self lastBtnMax]  + 10, 0, size.width, 40)];
    [headBtn setTitle:title forState:UIControlStateNormal];
    [headBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    headBtn.titleLabel.font = [UIFont systemFontOfSize:16];
    headBtn.tag = index;
    [headBtn addTarget:self action:@selector(nodeClick:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:headBtn];
    [self.headNodeViewArr addObject:headBtn];
    self.contentSize = CGSizeMake([self lastBtnMax],40);
    [self setContentOffset:CGPointMake([self withoffset], 0)];
}
-(void)nodeClick:(UIButton*)btn{
    NSInteger index = btn.tag;
    if (index < self.headNodeArr.count ) {
        NSDictionary* node =  [self.headNodeArr objectAtIndex:index];
        if (node) {
            NSString* type = [node objectForKey:@"type"];
            NSDictionary* payloadDic = [node  objectForKey:@"payload"];
            if ([type isEqualToString:@"1"] ) {
                NSString* parentId = [NSString stringWithFormat:@"%@",[payloadDic objectForKey:@"organizationId"]];
                if(self.nodeClickBlock){
                    self.nodeClickBlock(parentId);
                }
            }
        }
    }
    
    NSRange range = NSMakeRange(index + 1,self.headNodeArr.count - index - 1 );
    [self.headNodeArr removeObjectsInRange:range];
    
    NSMutableArray* deleteArr = [[NSMutableArray alloc]init];
    for (NSInteger i = index + 1; i < self.headNodeViewArr.count; i++) {
        UIButton* btn = [self.headNodeViewArr objectAtIndex:i];
        [deleteArr addObject:btn];
    }

    for (UIButton* btn in  deleteArr) {
        [btn removeFromSuperview];
    }
    [self.headNodeViewArr removeObjectsInRange:range];
    self.contentSize = CGSizeMake([self lastBtnMax],40);
    [self setContentOffset:CGPointMake([self withoffset], 0)];
}
-(CGFloat)lastBtnMax{
    if (self.headNodeViewArr.count > 0) {
        UIButton* lasBtn = [self.headNodeViewArr lastObject];
        CGFloat maxX = CGRectGetMaxX(lasBtn.frame);
        return maxX;
    }else{
        return 0;
    }
}
-(CGFloat)withoffset{
    
    if ([self lastBtnMax] > self.frame.size.width) {
        CGFloat with = [self lastBtnMax] - self.frame.size.width ;
        return  with;
    }else{
        return 0;
    }
}
/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

@end
