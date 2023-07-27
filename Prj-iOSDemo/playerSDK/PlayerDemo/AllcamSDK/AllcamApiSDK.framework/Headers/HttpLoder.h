//
//  HttpLoder.h
//  OneMail
//
//  Created by MC on 2017/7/2.
//  Copyright © 2017年 Chinasofti. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AFNetworking.h"
@interface HttpLoder : NSObject

+ (HttpLoder*) share;

- (AFHTTPSessionManager*) shopSessionManager;

-(void)clearShopSession;

+ (NSURLSessionTask*) doGet:(AFHTTPSessionManager*) manage Url:(NSString*) url Params:(NSDictionary*) params Success:(void (^)(NSURLSessionDataTask * task, id responseObject)) success failure:(void (^)(NSURLSessionDataTask * task, NSError * error))failure;

+ (NSURLSessionTask*) doPost:(AFHTTPSessionManager*) manage Url:(NSString*) url Params:(NSDictionary*) params CompletionHandler:(void (^)(NSURLSessionDataTask * task, id responseObject)) completionHandler;

+ (NSURLSessionTask*) doPut:(AFHTTPSessionManager*) manage Url:(NSString*) url Params:(NSDictionary*) params Success:(void (^)(NSURLSessionDataTask * task, id responseObject)) success failure:(void (^)(NSURLSessionDataTask * task, NSError * error))failure;

+ (NSURLSessionTask*) doPut2:(AFHTTPSessionManager*) manage Url:(NSString*) url Params:(NSDictionary*) params Success:(void (^)(NSURLSessionDataTask * task, id responseObject)) success failure:(void (^)(NSURLSessionDataTask * task, NSError * error))failure;

+ (NSURLSessionTask*) doDelete:(AFHTTPSessionManager*) manage Url:(NSString*) url Params:(NSDictionary*) params Success:(void (^)(NSURLSessionDataTask * task, id responseObject)) success failure:(void (^)(NSURLSessionDataTask * task, NSError * error))failure;

+ (NSURLSessionTask*) downloadTask:(AFURLSessionManager*) manage Request:(NSMutableURLRequest*) request toPath:(NSString*) path progress:(void (^)(NSProgress *downloadProgress)) downloadProgressBlock completionHandler:(void (^)(NSURL *filePath, NSError *error))completionHandler;

+ (NSURLSessionTask*) downloadResumeTask:(AFURLSessionManager*) manage Request:(NSMutableURLRequest*) request toPath:(NSString*) path progress:(void (^)(NSProgress *downloadProgress)) downloadProgressBlock completionHandler:(void (^)(NSURL *filePath, NSError *error))completionHandler;

+ (NSURLSessionTask*) uploadTask:(AFURLSessionManager*) manage Request:(NSURLRequest*) request FilePath:(NSString*) path progress:(void (^)(NSProgress *uploadProgress)) uploadProgressBlock completionHandler:(void (^)(NSURLResponse * response, id responseObject, NSError * error))completionHandler;

+ (NSURLSessionTask*) uploadDataTask:(AFURLSessionManager*) manage Request:(NSURLRequest*) request Data:(NSData*) data progress:(void (^)(NSProgress *uploadProgress)) uploadProgressBlock completionHandler:(void (^)(NSURLResponse * response, id responseObject, NSError * error))completionHandler;

- (void) getResultWithObject:(NSDictionary *)dic Task:(NSURLSessionDataTask *)task success:(void (^)( id))success failure:(void (^)( NSDictionary *))failure;
- (NSString *)md5:(NSString *)str;
-(NSString *)getAESString:(NSString *)string;
@end
