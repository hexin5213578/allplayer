//
//  AllcamPlayerViewController.m
//  PlayerDemo
//
//  Created by 秦骏 on 2022/1/7.
//

#import "AllcamPlayerViewController.h"
#import "HomePlayerView.h"
#import "Masonry.h"
@interface AllcamPlayerViewController ()<UITableViewDataSource,UITableViewDelegate>

@property (nonatomic, strong) HomePlayerView* localPlayerView;
//@property (nonatomic,strong) NSArray* dataArr;
@property (nonatomic, strong) NSMutableArray* dataArr;

@property (nonatomic, strong) UITableView* tableView;

@end

@implementation AllcamPlayerViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    self.dataArr = [[NSMutableArray alloc]init];
    self.title = @"本地播放";
    [self initUI];
    [self checkData];
}
-(void)initUI{
    [self.view addSubview:self.localPlayerView];
    [self.localPlayerView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.view.mas_top).offset(SafeAreaTopHeight);
        make.height.mas_equalTo([UIScreen mainScreen].bounds.size.width * 9/16);
        make.width.mas_equalTo([UIScreen mainScreen].bounds.size.width);
    }];
    [self.view addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.view.mas_left);
        make.bottom.equalTo(self.view.mas_bottom);
        make.right.equalTo(self.view.mas_right);
        make.top.equalTo(self.localPlayerView.mas_bottom);
    }];
}
-(void)checkData{
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
   NSArray* arr = [fileManager contentsOfDirectoryAtPath:outputFilePath error:&error];

    for( NSString* filename in arr ){
        if ([filename hasSuffix:@"mp4"]) {
            [self.dataArr addObject:filename];
        }
    }
    
    if (error) {
        NSLog(@"%@",error.localizedDescription);
    }else{
        [self.tableView reloadData];
    }
}
-(HomePlayerView*)localPlayerView{
    if (!_localPlayerView) {
        _localPlayerView = [[HomePlayerView alloc]initWithType:playerView_local];
        _localPlayerView.backgroundColor = [UIColor blackColor];
        _localPlayerView.parentVC = self;
    }
    return _localPlayerView;
}
-(UITableView*)tableView{
    if (!_tableView) {
        _tableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, 0 , 0) style:UITableViewStylePlain];
        [_tableView setDelegate:self];
        [_tableView setDataSource:self];
    }
    return _tableView;
}
- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    static NSString * carReuseIdentifier = @"UITableViewCell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:carReuseIdentifier];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:carReuseIdentifier];
    }
    NSString* dataName = [self.dataArr objectAtIndex:indexPath.row];
    cell.textLabel.text = dataName;
    return cell;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.dataArr.count;
}
-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{
    
   return 1;
}

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath{
    NSString* filename = [self.dataArr objectAtIndex:indexPath.row];
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:filename];
    [self.localPlayerView selectVideo:outputFileName];

}

@end
