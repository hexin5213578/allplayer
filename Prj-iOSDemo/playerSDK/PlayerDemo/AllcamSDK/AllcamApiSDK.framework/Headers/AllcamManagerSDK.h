//
//  AllcamManagerSDK.h
//  AllcamSDK
//
//  Created by 秦骏 on 2021/11/23.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AllcamManagerSDK : NSObject

/** 配置Api环境
@param configIp  IP地址
@param port 端口  http 10000  https 10002
 */

/**  配置Api环境
 @param configIp  IP地址
 @param port 端口  http 10000  https 10002
 @param acount 账户
 @param way 验证方式
 */
+(void)initSDKWithConfigIp:(NSString*)configIp AndPort:(NSString*)port Acount:(NSString*)acount Way:(NSString*)way;
+(void)updateSDKWithConfigSite:(NSString*)domain Acount:(NSString*)acount Way:(NSString*)way;
+(void)updateSDKWithConfigSite:(NSString*)domain;
+(void)initSDKWithConfigIp:(NSString*)configIp AndPort:(NSString*)port;
+(NSMutableDictionary*)initWithParams;
+(NSString *)getDomain;
+(NSString *)getClientNonce;
+(NSString *)getIp;
+(NSString *)getPort;
+(NSString*)getAccount;
+(NSString*)getPassword;
/** 获取图片验证码
 @param success 返回验证码
 @param failure 失败回调
 */
+(void)getCodeSuccess:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/// /** 获取图片验证码
/// @param api 对应接口请求路径
/// @param success 返回验证码
/// @param failure 失败回调
+(void)getCode:(NSString*)api Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 登录
 @param acount 账号
 @param password 密码
 @param code 验证码
 @param success 返回用户信息
 @param failure 登录失败提示
 */
+(void)loginWithAcount:(NSString*)acount AndPassword:(NSString*)password cid:(NSString*)cid Code:(NSString*)code Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;
+(void)loginWithAcount:(NSString*)acount AndPassword:(NSString*)password Code:(NSString*)code Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 获取AppStore版本号
 @param baseurl 苹果官网地址 http://itunes.apple.com/cn
 @param appleId 线上版本的appleid
 @param success 返回 苹果app基本信息
 @param failure 失败回调
 */
+(void)checkAppStoreVersionWithBaseUrl:(NSString*)baseurl appleId:(NSString*)appleId Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/* 获取token
 @param refreshToken 刷新token  (可选)
 @param success 返回token
 @param failure 失败回调
 */
+(void)getToken:(NSString*)accessToken Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/* 退出登录
 @param cid 客户端id
 @param success 退出登录
 @param failure 失败回调
 */
+(void)logout:(NSString*)cid  Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;



@end

NS_ASSUME_NONNULL_END
