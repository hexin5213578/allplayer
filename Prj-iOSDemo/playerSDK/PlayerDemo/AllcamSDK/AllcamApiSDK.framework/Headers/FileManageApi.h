//
//  FileManageApi.h
//  SmartShop
//
//  Created by 秦骏 on 2021/12/14.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface FileManageApi : NSObject
/// 获取上传文件地址
/// @param success 数据返回
/// @param failure 错误回调
+ (void)getUploadFileUrlSuccess:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/// 上传文件
/// @param data 文件数据
/// @param fileId 文件id
/// @param uploadUrl 上传路径
/// @param type 文件类型
/// @param success 数据返回
/// @param failure 错误回调
+(void) uploadData:(NSData*)data FileId:(NSString *)fileId toUrl:(NSString *)uploadUrl Type:(NSInteger)type filename:(NSString*)name Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;
@end

NS_ASSUME_NONNULL_END
