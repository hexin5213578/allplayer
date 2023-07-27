//
//  main.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/16.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "ViewController.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
