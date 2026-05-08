// MyMetalView.mm
#import "MyMetalView.h"
#import "MetalRenderer.h"

@implementation MyMetalView {
    MetalRenderer *_renderer;
    NSPoint        _lastMouseLocation;
    BOOL           _firstMouseMove;
}

- (instancetype)initWithFrame:(NSRect)frame {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"❌ Failed to create Metal device");
        return nil;
    }

    self = [super initWithFrame:frame device:device];
    if (!self) return nil;

    self.colorPixelFormat        = MTLPixelFormatBGRA8Unorm;
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    self.preferredFramesPerSecond = 60;

    _renderer    = [[MetalRenderer alloc] initWithMTKView:self];
    self.delegate = _renderer;

    NSLog(@"🎮 Controls: WASD=move, Space=up, Q=down, R=reset, Right-click+drag=look");
    return self;
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyView      { return YES; }

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) [self.window makeFirstResponder:self];
}

// ---- Keyboard -------------------------------------------------------------

- (void)keyDown:(NSEvent *)event { [_renderer keyDown:event.keyCode]; }
- (void)keyUp:(NSEvent *)event   { [_renderer keyUp:event.keyCode]; }

// ---- Right-mouse drag drives look ----------------------------------------

- (void)rightMouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];
    [_renderer setMouseLookEnabled:YES];
    _firstMouseMove    = YES;
    _lastMouseLocation = [self convertPoint:event.locationInWindow fromView:nil];
}

- (void)rightMouseUp:(NSEvent *)event {
    [_renderer setMouseLookEnabled:NO];
}

- (void)rightMouseDragged:(NSEvent *)event {
    NSPoint current = [self convertPoint:event.locationInWindow fromView:nil];
    if (!_firstMouseMove) {
        const float dx = current.x - _lastMouseLocation.x;
        const float dy = current.y - _lastMouseLocation.y;
        [_renderer processMouseDeltaX:dx deltaY:dy];
    } else {
        _firstMouseMove = NO;
    }
    _lastMouseLocation = current;
}

@end
