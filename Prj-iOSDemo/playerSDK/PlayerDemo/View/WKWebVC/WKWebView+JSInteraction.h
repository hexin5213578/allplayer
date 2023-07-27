//
//  WKWebView+JSInteraction.h
//  WKWebViewDemo
//
//  Created by 秦骏 on 2021/11/15.
//

#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface WKWebView (JSInteraction)
- (NSString *)dicToString:(id)responseObject;
@end

NS_ASSUME_NONNULL_END
