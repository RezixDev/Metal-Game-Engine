// MyMetalView.mm
#import "MyMetalView.h"
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#import "MetalApplication.hpp"
#import "Engine/Core/Time.hpp"

extern "C" void AppInitWithMetal(id<MTLDevice> device, CAMetalLayer *layer);

@implementation MyMetalView {
    CAMetalLayer *_metalLayer;
    id<MTLDevice> _device;
    dispatch_source_t _renderTimer;

    MetalApplication *_application;

    NSPoint _lastMouseLocation;
    BOOL _firstMouseMove;
}

+ (Class)layerClass { return [CAMetalLayer class]; }

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return nil;

    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        NSLog(@"❌ Failed to create Metal device");
        return nil;
    }

    CGFloat scale = NSScreen.mainScreen.backingScaleFactor;

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device             = _device;
    _metalLayer.pixelFormat        = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly    = YES;
    _metalLayer.contentsScale      = scale;
    _metalLayer.displaySyncEnabled = YES;
    _metalLayer.drawableSize       = CGSizeMake(frame.size.width  * scale,
                                                frame.size.height * scale);

    self.layer       = _metalLayer;
    self.wantsLayer  = YES;

    _application = &MetalApplication::shared();

    try {
        AppInitWithMetal(_device, _metalLayer);
    } catch (const std::exception& e) {
        NSLog(@"❌ Failed to initialize application: %s", e.what());
        return nil;
    }

    // Hand the initial drawable size to the renderer so viewport/aspect
    // are correct from the very first frame.
    _application->onResize(_metalLayer.drawableSize.width,
                           _metalLayer.drawableSize.height);

    Engine::Core::Time::getInstance().initialize();

    _firstMouseMove = YES;
    [self setupRenderTimer:60.0];

    NSLog(@"🎮 Controls: WASD=move, Space=up, Q=down, R=reset, Right-click+drag=look");
    return self;
}

- (void)setupRenderTimer:(double)targetFPS {
    uint64_t intervalNanos = (uint64_t)(NSEC_PER_SEC / targetFPS);

    dispatch_queue_t queue = dispatch_queue_create("com.metalengine.render", DISPATCH_QUEUE_SERIAL);
    _renderTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

    dispatch_source_set_timer(_renderTimer,
                              dispatch_time(DISPATCH_TIME_NOW, 0),
                              intervalNanos,
                              intervalNanos / 10);

    __block __typeof(self) blockSelf = self;
    dispatch_source_set_event_handler(_renderTimer, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            [blockSelf renderFrame];
        });
    });

    dispatch_resume(_renderTimer);
}

- (void)renderFrame {
    if (!_application) return;

    Engine::Core::Time::getInstance().update();
    float deltaTime = Engine::Core::Time::getInstance().getDeltaTime();

    _application->onUpdate(deltaTime);
    _application->onRender();
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyView      { return YES; }

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        [self.window makeFirstResponder:self];
    }
}

// ========================================
// Keyboard
// ========================================

- (void)keyDown:(NSEvent *)event { _application->processKeyDown(event.keyCode); }
- (void)keyUp:(NSEvent *)event   { _application->processKeyUp(event.keyCode); }

// ========================================
// Mouse — right button drives look
// ========================================

- (void)rightMouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];
    _application->setMouseLook(YES);
    _firstMouseMove    = YES;
    _lastMouseLocation = [self convertPoint:event.locationInWindow fromView:nil];
}

- (void)rightMouseUp:(NSEvent *)event {
    _application->setMouseLook(NO);
}

- (void)rightMouseDragged:(NSEvent *)event {
    if (!_application->getInputState().mouseLookEnabled) return;

    NSPoint current = [self convertPoint:event.locationInWindow fromView:nil];
    if (!_firstMouseMove) {
        float dx = current.x - _lastMouseLocation.x;
        float dy = current.y - _lastMouseLocation.y;
        _application->processMouseMove(dx, dy);
    } else {
        _firstMouseMove = NO;
    }
    _lastMouseLocation = current;
}

// ========================================
// Resize
// ========================================

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];

    CGFloat scale = self.window.backingScaleFactor ?: 1.0;
    CGSize newSize = CGSizeMake(self.bounds.size.width  * scale,
                                self.bounds.size.height * scale);
    _metalLayer.drawableSize = newSize;
    _application->onResize((int)newSize.width, (int)newSize.height);
}

// ========================================
// Cleanup
// ========================================

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
    if (newWindow == nil) {
        [self cleanup];
    }
    [super viewWillMoveToWindow:newWindow];
}

- (void)cleanup {
    if (_renderTimer) {
        dispatch_source_cancel(_renderTimer);
        _renderTimer = nil;
    }
    if (_application) {
        _application->onShutdown();
        _application = nullptr;
    }
    _metalLayer = nil;
    _device = nil;
}

- (void)dealloc {
    [self cleanup];
    [super dealloc];
}

@end
