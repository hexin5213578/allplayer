//
//  DeviceListView.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/17.
//

#import "DeviceListView.h"
#import "HeadScrollView.h"
#import "AllcamApi.h"
#import "Masonry.h"
@interface DeviceListView()<UITableViewDataSource,UITableViewDelegate>
@property(nonatomic, strong) HeadScrollView* headScrollView;

@property(nonatomic, strong)UITableView* tableView;
@property(nonatomic, strong)NSMutableArray* dataSource;

@property(nonatomic, strong) NSDictionary* selectedNodeDic;
@property(nonatomic, strong) NSDictionary* playDic;
@end
@implementation DeviceListView

-(instancetype)initWithFrame:(CGRect)frame{
    if (self = [super initWithFrame:frame]) {
        [self initUI];
    }
    return self;
}
-(void)initUI{
    [self addSubview:self.headScrollView];
    [self.headScrollView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.mas_left).offset(10);
        make.height.offset(40);
        make.right.equalTo(self.mas_right).offset(-10);
        make.top.equalTo(self.mas_top);
    }];

    [self addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.mas_left);
        make.bottom.equalTo(self.mas_bottom);
        make.right.equalTo(self.mas_right);
        make.top.equalTo(self.headScrollView.mas_bottom);
    }];
}
-(void)getDeviceList:(NSString*)parentId{
  
    __weak typeof(self) weakself = self;
    [AllcamApi getDeviceListWithType:@"1" ParentId:parentId SubUserId:@"" DeviceType:@"" Success:^(NSDictionary * _Nonnull result) {
      
        weakself.dataSource = result[@"nodeList"];
        [weakself.tableView reloadData];
    } failure:^(NSDictionary * _Nonnull error) {
       
    }];
}
-(HeadScrollView*)headScrollView{
    if (!_headScrollView) {
        _headScrollView = [[HeadScrollView alloc]init];
        __weak typeof(self) weakSelf = self;
        _headScrollView.nodeClickBlock = ^(NSString * _Nonnull parentId) {
            [weakSelf getDeviceList:parentId];
        };
        _headScrollView.getDeviceBlock = ^{
            [weakSelf getDeviceList:@""];
        };
    }
    return _headScrollView;
    
}
-(UITableView*)tableView{
    if (!_tableView) {
        _tableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, 0 , 0) style:UITableViewStylePlain];
        [_tableView setDelegate:self];
        [_tableView setDataSource:self];
    }
    return _tableView;
}
#pragma UITableView 代理
- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    static NSString * carReuseIdentifier = @"UITableViewCell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:carReuseIdentifier];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:carReuseIdentifier];
    }
    NSDictionary* modeDic = [self.dataSource objectAtIndex:indexPath.row];
    NSDictionary* payloadDic = [modeDic objectForKey:@"payload"];
    NSString* type = [modeDic objectForKey:@"type"];
    if ([type isEqualToString:@"1"] ) {
        cell.textLabel.text = [NSString stringWithFormat:@"组:%@",[payloadDic objectForKey:@"organizationName"]];
    }else if([type isEqualToString:@"2"]){
        cell.textLabel.text = [NSString stringWithFormat:@"镜头:%@",[payloadDic objectForKey:@"deviceName"]];;
    }
    
    return cell;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.dataSource.count;
}
-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{
    
   return 1;
}

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath{
    self.selectedNodeDic = [self.dataSource objectAtIndex:indexPath.row];
    NSString* type = [ self.selectedNodeDic objectForKey:@"type"];
    if ([type isEqualToString:@"1"]) {
        [self.headScrollView addhHead: self.selectedNodeDic];
        if (self.selectedNodeDic) {
            NSString* type = [self.selectedNodeDic objectForKey:@"type"];
            NSDictionary* payloadDic = [self.selectedNodeDic  objectForKey:@"payload"];
            if ([type isEqualToString:@"1"] ) {
                NSString* parentId = [NSString stringWithFormat:@"%@",[payloadDic objectForKey:@"organizationId"]];
                [self getDeviceList:parentId];
            }
        }
    }else{
        if(self.selectedNodeBlock){
            NSDictionary* payloadDic = [self.selectedNodeDic  objectForKey:@"payload"];
            self.selectedNodeBlock(payloadDic);
        }
    }
}

@end
