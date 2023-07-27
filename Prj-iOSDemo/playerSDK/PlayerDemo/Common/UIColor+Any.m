//
//  UIColor+Any.m
//  SmartShop
//
//  Created by 秦骏 on 2021/12/9.
//

#import "UIColor+Any.h"

@implementation UIColor (Any)
+(UIColor*)themeColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"themeColor"];
        return color;
    } else {
        return UIColorFromHex(0xFFA340);
    }
}
+(UIColor*)whiteColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"whiteColor"];
        return  color;
    } else {
        return  UIColor.whiteColor;
    }
}
+(UIColor*)blackColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"blackColor"];
        return  color;
    } else {
        return  UIColor.blackColor;
    }
}
+(UIColor*)greenColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"greenColor"];
        return  color;
    } else {
        return  UIColor.greenColor;
    }
}
+(UIColor*)lineColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"lineColor"];
        return  color;
    } else {
        return  UIColor.lineColor;
    }
}
+(UIColor*)tipColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"tipColor"];
        return  color;
    } else {
        return   UIColorFromHex(0x808080);
    }
}
+(UIColor*)bgColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"bgColor"];
        return  color;
    } else {
        return   UIColorFromHex(0xF2F2F2);
    }
}
+(UIColor*)itemtitleColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"itemtitleColor"];
        return  color;
    } else {
        return   UIColorFromHex(0x4D4D4D);
    }
}
+(UIColor*)maskColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"maskColor"];
        return  color;
    } else {
        return   UIColorFromHex(0x000000);
    }
}
+(UIColor*)cardColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"cardColor"];
        return  color;
    } else {
        return   UIColorFromHex(0x0A0A0A);
    }
}
+(UIColor*)deleteColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"deleteColor"];
        return  color;
    } else {
        return   UIColorFromHex(0xFE5E5E);
    }
}
+(UIColor*)valueColor{
    if (@available(iOS 11.0, *)) {
        UIColor* color = [UIColor colorNamed:@"valueColor"];
        return  color;
    } else {
        return   UIColorFromHex(0x999999);
    }
}
@end
