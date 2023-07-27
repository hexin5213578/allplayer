//
//  WKWebView+JSInteraction.m
//  WKWebViewDemo
//
//  Created by 秦骏 on 2021/11/15.
//

#import "WKWebView+JSInteraction.h"

@implementation WKWebView (JSInteraction)
- (NSString *)dicToString:(id)responseObject{
    NSError *error;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:responseObject options:NSJSONWritingPrettyPrinted error:&error];
    NSString *responseJson = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    
    NSMutableString *mutStr = [NSMutableString stringWithString:responseJson];

       NSRange range = {0,responseJson.length};

       //去掉字符串中的空格

       [mutStr replaceOccurrencesOfString:@" " withString:@"" options:NSLiteralSearch range:range];

       NSRange range2 = {0,mutStr.length};

       //去掉字符串中的换行符

       [mutStr replaceOccurrencesOfString:@"\n" withString:@"" options:NSLiteralSearch range:range2];
    
    if (error) {
        return @"";
    }else{
       return mutStr;
    }
}
@end
