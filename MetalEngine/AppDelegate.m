// AppDelegate.m
#import "AppDelegate.h"
#import "MyMetalView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 960, 800);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled
                                | NSWindowStyleMaskClosable
                                | NSWindowStyleMaskResizable
                                | NSWindowStyleMaskMiniaturizable;

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:styleMask
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"MetalEngine";
    self.window.minSize = NSMakeSize(400, 300);
    [self.window center];

    NSRect viewFrame = ((NSView *)self.window.contentView).bounds;
    self.metalView = [[MyMetalView alloc] initWithFrame:viewFrame];
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    self.window.contentView = self.metalView;
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.metalView];

    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end
