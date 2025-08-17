//m
#import <AppKit/AppKit.h>
#import "MyMetalView.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MyMetalView *metalView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create the main window programmatically
    NSRect frame = NSMakeRect(0, 0, 960, 800);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskResizable |
                                  NSWindowStyleMaskMiniaturizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                               styleMask:styleMask
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    
    // Configure window
    self.window.title = @"Metal + AppKit + WASD";
    [self.window center];
    self.window.minSize = NSMakeSize(400, 300);

    // Create and setup Metal view
    NSRect viewFrame = ((NSView*)self.window.contentView).bounds;
    self.metalView = [[MyMetalView alloc] initWithFrame:viewFrame];
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Set as content view
    self.window.contentView = self.metalView;
    
    // Show window and make it key
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.metalView];

    NSLog(@"Application launched - single window created");
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"Application terminating");

}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end
