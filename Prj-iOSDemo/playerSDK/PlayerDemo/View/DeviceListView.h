//
//  DeviceListView.h
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/17.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
typedef void (^SelectedNodeBlock)(NSDictionary* nodeDic);
@interface DeviceListView : UIView

@property (readwrite, nonatomic, copy) SelectedNodeBlock selectedNodeBlock;

-(void)getDeviceList:(NSString*)parentId;

@end

NS_ASSUME_NONNULL_END
