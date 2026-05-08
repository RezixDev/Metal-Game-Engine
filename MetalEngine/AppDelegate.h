// AppDelegate.h
#import <AppKit/AppKit.h>

@class MyMetalView;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MyMetalView *metalView;
@end
