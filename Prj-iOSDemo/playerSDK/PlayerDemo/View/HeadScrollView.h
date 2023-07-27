//
//  HeadScrollView.h
//  AllcamApiDemo
//
//  Created by 秦骏 on 2021/12/1.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
typedef void (^NodeClickBlock)(NSString* parentId);

typedef void (^GetDeviceBlock)(void);

@interface HeadScrollView : UIScrollView

@property (readwrite, nonatomic, copy) NodeClickBlock nodeClickBlock;

@property (readwrite, nonatomic, copy) GetDeviceBlock getDeviceBlock;

@property (nonatomic, strong)UIButton* deviceBtn;

@property(nonatomic, strong)NSMutableArray* headNodeArr;

@property(nonatomic, strong)NSMutableArray* headNodeViewArr;

-(void)addhHead:(NSDictionary*)nodeDic;
@end

NS_ASSUME_NONNULL_END
