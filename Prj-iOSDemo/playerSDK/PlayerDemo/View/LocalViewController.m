//
//  LocalViewController.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/27.
//

#import "LocalViewController.h"
#import "UIColor+Any.h"
#import "Masonry.h"
#import "SiglePlayer.h"
#import "WKWebViewController.h"
#import <CoreMedia/CoreMedia.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import <AVFoundation/AVFoundation.h>
#import "SystemPlayerViewController.h"
#import "FileManageApi.h"
#import "AllcamPlayerViewController.h"
@interface LocalViewController ()<UITableViewDataSource,UITableViewDelegate,UINavigationControllerDelegate, UIImagePickerControllerDelegate>

@property (nonatomic,strong) NSArray* dataArr;

@property (nonatomic, strong) UITableView* tableView;

@property (nonatomic, strong) UIView* headView;

@property (nonatomic, strong) UIImageView* imageView;

@property (nonatomic, strong) SiglePlayer* player;
@property (nonatomic, strong) OpenGLView20* playerView;

@property (nonatomic, strong) UIButton* openBtn;
@property (nonatomic, strong) UIButton* localPlayBtn;


@end

@implementation LocalViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"本地存储数据";
    self.view.backgroundColor = [UIColor whiteColor];
    self.player = [[SiglePlayer alloc]init];
  
    self.navigationItem.rightBarButtonItems = @[[[UIBarButtonItem alloc] initWithCustomView:self.openBtn],[[UIBarButtonItem alloc] initWithCustomView:self.localPlayBtn]];
    [self initUI];
    [self checkData];
}
-(UIButton*)openBtn{
    if(!_openBtn){
        _openBtn = [[UIButton alloc]init];
        [_openBtn setTitle:@"系统资源" forState:UIControlStateNormal];
        _openBtn.backgroundColor = [UIColor whiteColor];
        [_openBtn setTitleColor: [UIColor themeColor] forState:UIControlStateNormal];
        _openBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_openBtn addTarget:self action:@selector(openPress) forControlEvents:UIControlEventTouchUpInside];
    }
    return _openBtn;
}
-(UIButton*)localPlayBtn{
    if(!_localPlayBtn){
        _localPlayBtn = [[UIButton alloc]init];
        [_localPlayBtn setTitle:@"本地播放" forState:UIControlStateNormal];
        _localPlayBtn.backgroundColor = [UIColor whiteColor];
        [_localPlayBtn setTitleColor: [UIColor themeColor] forState:UIControlStateNormal];
        _localPlayBtn.titleLabel.font = [UIFont systemFontOfSize:15.0];
        [_localPlayBtn addTarget:self action:@selector(localPress) forControlEvents:UIControlEventTouchUpInside];
    }
    return _localPlayBtn;
}

-(void)openPress{
    [self showAlertSheet];
}
-(void)localPress{
    AllcamPlayerViewController* localplayer = [[AllcamPlayerViewController alloc]init];
    [self.navigationController pushViewController:localplayer animated:YES];
}

-(void)checkData{
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
    self.dataArr = [fileManager contentsOfDirectoryAtPath:outputFilePath error:&error];
    if (error) {
        NSLog(@"%@",error.localizedDescription);
    }else{
        [self.tableView reloadData];
    }
}

-(void)initUI{
    [self.view addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.equalTo(self.view.mas_left);
        make.bottom.equalTo(self.view.mas_bottom);
        make.right.equalTo(self.view.mas_right);
        make.top.equalTo(self.view.mas_top);
    }];
}
-(UIImageView*)imageView{
    if (!_imageView) {
        _imageView = [[UIImageView alloc]initWithFrame:CGRectMake(0, 0, self.view.frame.size.width,  self.view.frame.size.width * 9 /16)];
    }
    return _imageView;
}
-(UIView*)headView{
    if (!_headView) {
        _headView = [[UIImageView alloc]initWithFrame:CGRectMake(0, 0, self.view.frame.size.width,  self.view.frame.size.width * 9 /16)];
    }
    return _headView;
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
    NSString* dataName = [self.dataArr objectAtIndex:indexPath.row];
    [self catFile:dataName];
}
-(void)catFile:(NSString*)filename{
    if ([filename hasSuffix:@"jpg"]) {
        NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
        NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:filename];
        self.imageView.image = [UIImage imageWithContentsOfFile:outputFileName];
        self.tableView.tableHeaderView = self.imageView;
        [self.tableView reloadData];
    }else if ([filename hasSuffix:@"mp4"]){
        [self.player localStop];
        [self.playerView removeFromSuperview];
        NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
        NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:filename];
        [self showPlayerSheet:outputFileName filename:filename];
    }else if ([filename hasSuffix:@"log"]){
        NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
        NSString *outputFileName = [[outputFilePath stringByAppendingString:@"/"] stringByAppendingString:filename];
        [self showLogSheet:outputFileName filename:filename];
    }
}
-(void)showLogSheet:(NSString*)filePath filename:(NSString*)filename{
    __weak typeof(self) weakSelf = self;
   UIAlertController *alertController = [[UIAlertController alloc] init];
   UIAlertAction *cancle = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
   }];
   UIAlertAction *checklog = [UIAlertAction actionWithTitle:@"查看" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
       [weakSelf catLog:filePath filename:filename];
   }];
   UIAlertAction *deletelog = [UIAlertAction actionWithTitle:@"清空" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
       [weakSelf deleteLog:filePath];
       
   }];
  
   [alertController addAction:cancle];
   [alertController addAction:checklog];
   [alertController addAction:deletelog];
    NSString * filenameurl = [[NSUserDefaults standardUserDefaults]objectForKey:@"filename"];
    if (filenameurl) {
        UIAlertAction *online = [UIAlertAction actionWithTitle:@"在线播放" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
           
        }];
        [alertController addAction:online];
    }
   [self presentViewController:alertController animated:true completion:nil];
}
-(void)catLog:(NSString*)filePath filename:(NSString*)filename{
    WKWebViewController* wkWebVC = [[WKWebViewController alloc]init];
    wkWebVC.title = filename;
    wkWebVC.webUrl = filePath;
    [self.navigationController pushViewController:wkWebVC animated:YES];
}
-(void)deleteLog:(NSString*)filePath{
    NSString* content = @"";
    NSError* error;
    [content writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&error];
    if (error) {
        NSLog(@"写入失败");
    }
}

-(void)showPlayerSheet:(NSString*)mediaUrl filename:( NSString*) filename{
    __weak typeof(self) weakSelf = self;
   UIAlertController *alertController = [[UIAlertController alloc] init];
   UIAlertAction *cancle = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
   }];
   UIAlertAction *lib = [UIAlertAction actionWithTitle:@"奥看播放" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
       [weakSelf allcamPlay:mediaUrl];
   }];
   UIAlertAction *camera = [UIAlertAction actionWithTitle:@"系统播放" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
       [weakSelf systemPlay:mediaUrl];
       
   }];
    UIAlertAction *uplaod = [UIAlertAction actionWithTitle:@"上传文件" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [weakSelf uplaodfile:mediaUrl Type:2 filename:filename];
    }];
   
   [alertController addAction:cancle];
   [alertController addAction:lib];
   [alertController addAction:camera];
   [alertController addAction:uplaod];
    NSString * filenameurl = [[NSUserDefaults standardUserDefaults]objectForKey:@"filename"];
    if (filenameurl) {
        UIAlertAction *online = [UIAlertAction actionWithTitle:@"在线播放" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
           
        }];
        [alertController addAction:online];
    }
   [self presentViewController:alertController animated:true completion:nil];
}
-(void)allcamPlay:(NSString*)mediaUrl{
    __weak typeof(self) weakself = self;
    [self.player loacalPlay:mediaUrl width:self.view.frame.size.width done:^(OpenGLView20 * _Nonnull playew, BOOL done) {
        dispatch_async(dispatch_get_main_queue(), ^{
            weakself.playerView = playew;
            weakself.tableView.tableHeaderView = weakself.headView;
            [weakself.headView addSubview:weakself.playerView];
            [weakself.player loacalPlay];
            weakself.playerView.frame = CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.width * 9 /16);
            [weakself.tableView reloadData];
        });
    }];
}
-(void)systemPlay:(NSString*)mediaUrl{
    SystemPlayerViewController*  sysplayer = [[SystemPlayerViewController alloc]init];
    sysplayer.mediaUrl = mediaUrl;
    [self.navigationController pushViewController:sysplayer animated:YES];
}
-(void)uplaodfile:(NSString*)outputfile Type:(NSInteger)type filename:(NSString*)name{
    
    [FileManageApi getUploadFileUrlSuccess:^(NSDictionary * _Nonnull result) {
        NSData* fileData = [NSData dataWithContentsOfFile:outputfile];
        NSString* url = result[@"uploadUrl"];
        NSString* fileId = result[@"fileId"];
       
        [FileManageApi uploadData:fileData FileId:fileId toUrl:url  Type:type filename:name Success:^(NSDictionary * _Nonnull result) {
            [[NSUserDefaults standardUserDefaults] setObject:url forKey:@"filename"];
        } failure:^(NSDictionary * _Nonnull error) {
         
        }];
       
    } failure:^(NSDictionary * _Nonnull error) {
            
    }];
}
-(void)showAlertSheet{
    __weak typeof(self) weakSelf = self;
   UIAlertController *alertController = [[UIAlertController alloc] init];
   UIAlertAction *cancle = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
   }];
   UIAlertAction *lib = [UIAlertAction actionWithTitle:@"相册" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
       [weakSelf libraryPhoto];
   }];
   UIAlertAction *camera = [UIAlertAction actionWithTitle:@"相机" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
    [weakSelf takePhoto];
   }];
   [alertController addAction:cancle];
   [alertController addAction:lib];
   [alertController addAction:camera];
   [self presentViewController:alertController animated:true completion:nil];
}
-(void)takePhoto {
    if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera]) {
        UIImagePickerController *picker = [[UIImagePickerController alloc] init];
        picker.delegate = self;
        picker.sourceType = UIImagePickerControllerSourceTypeCamera;
        picker.mediaTypes = @[(NSString *)kUTTypeMovie,(NSString *)kUTTypeImage];
        picker.modalPresentationStyle = UIModalPresentationOverFullScreen;
        [self presentViewController:picker animated:YES completion:^{}];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            
        });
    }
}

-(void)libraryPhoto {
    if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]) {
        UIImagePickerController *picker = [[UIImagePickerController alloc] init];
        picker.delegate = self;
        picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
        picker.mediaTypes = @[(NSString *)kUTTypeMovie,(NSString *)kUTTypeImage];
        picker.modalPresentationStyle = UIModalPresentationOverFullScreen;
        [self presentViewController:picker animated:YES completion:^{
           
        }];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            
        });
    }
}
#pragma mark UIImagePickerControllerDelegate
-(void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info{
    NSString *outputFilePath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *mediaType = [info objectForKey:UIImagePickerControllerMediaType];
    if ([mediaType isEqualToString:(NSString *)kUTTypeImage]) {
        UIImage *orginal = [info objectForKey:UIImagePickerControllerEditedImage];
        [self saveImage:orginal outputPath:outputFilePath fileName:[self getfilename]];
    }else if ([mediaType isEqualToString:(NSString *)kUTTypeMovie]){
        NSURL* mediaURL = [info objectForKey:UIImagePickerControllerMediaURL];
        [self saveVideo:mediaURL output:outputFilePath fileName:[self getfilename]];
    }
    [self dismissViewControllerAnimated:YES completion:^{
    }];
}
-(void)saveVideo:(NSURL*)videoUrl output:(NSString*)outputPath fileName:(NSString *)fileName{
    AVURLAsset *urlAsset = [AVURLAsset URLAssetWithURL:videoUrl options:nil];
    NSArray *exportArray = [AVAssetExportSession exportPresetsCompatibleWithAsset:urlAsset];
    NSLog(@"exportArray = %@",exportArray);
    AVAssetExportSession *session = [[AVAssetExportSession alloc] initWithAsset:urlAsset presetName:AVAssetExportPresetHighestQuality];
       
    if (![[NSFileManager defaultManager] fileExistsAtPath:outputPath isDirectory:nil]) {
            [[NSFileManager defaultManager] createDirectoryAtPath:outputPath withIntermediateDirectories:YES attributes:nil error:nil];
        }
    ;
    NSString * savePath = [[NSString stringWithFormat:@"%@/%@",outputPath,fileName] stringByAppendingFormat:@".mp4"];
    [session setOutputURL:[NSURL fileURLWithPath:savePath]];
    [session setOutputFileType:AVFileTypeQuickTimeMovie];
    [session exportAsynchronouslyWithCompletionHandler:^{
            switch ([session status]) {
                    case AVAssetExportSessionStatusCompleted:
                        NSLog(@"转吗完成");
                        break;
                    case AVAssetExportSessionStatusFailed:
                        NSLog(@"转吗失败 %@，，，，%@",[[session error] localizedDescription],[session error]);
                    default:
                        break;
                }
            }];
}
-(BOOL)saveImage:(UIImage *)image outputPath:(NSString *)outputPath fileName:(NSString *)fileName
{
    NSData* imageData = UIImagePNGRepresentation(image);
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL existed = [fileManager fileExistsAtPath:outputPath ];
    BOOL isCreated = NO;
     NSString *path= [outputPath stringByAppendingPathComponent:[NSString stringWithFormat:@"/%@",fileName]];
    if ( existed )
    {
        [fileManager createFileAtPath:path contents:nil attributes:nil];
        isCreated =  [imageData writeToFile:path atomically:YES];
    }
    return isCreated;
}
-(NSString*)getfilename{
    NSDate *currentDate = [NSDate date];
    NSDateFormatter *dataFormatter = [[NSDateFormatter alloc]init];
    [dataFormatter setDateFormat:@"HH_mm_ss"];
    NSString *dateString = [dataFormatter stringFromDate:currentDate];
    return dateString;
}
@end
