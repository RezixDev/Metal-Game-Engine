// MyMetalView.mm
#import "MyMetalView.h"
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// Include C++ headers in the implementation only
#import "Renderer.hpp"
#import "MetalApplication.hpp"
#import "Engine/Core/Time.hpp"
#import "Engine/Camera.hpp"

// Use the Camera type alias from the global namespace
using Camera = Engine::Camera;

@implementation MyMetalView {
    CAMetalLayer* _metalLayer;
    id<MTLDevice> _device;
    Renderer* _renderer;
    dispatch_source_t _renderTimer;

    // Application integration
    MetalApplication* _application;

    // Mouse tracking
    NSPoint _lastMouseLocation;
    BOOL _firstMouseMove;
}

+ (Class)layerClass { return [CAMetalLayer class]; }

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return self;

    NSLog(@"🚀 Starting MyMetalView initialization with new architecture...");

    // Initialize Metal
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        NSLog(@"❌ Failed to create Metal device");
        return nil;
    }
    NSLog(@"✅ Metal device created: %@", _device.name);

    // Setup Metal layer
    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = YES;
    _metalLayer.contentsScale = [NSScreen mainScreen].backingScaleFactor;
    _metalLayer.displaySyncEnabled = YES;

    self.layer = _metalLayer;
    self.wantsLayer = YES;

    // Initialize renderer
    _renderer = [[Renderer alloc] initWithDevice:_device layer:_metalLayer];
    if (!_renderer) {
        NSLog(@"❌ Failed to create renderer");
        return nil;
    }

    // Initialize application
    _application = getMetalApplication();
    _application->onInitialize();

    // Initialize time system
    Engine::Core::Time::getInstance().initialize();

    _firstMouseMove = YES;

    // Create render timer for 60 FPS
    [self setupRenderTimer:60.0];

    NSLog(@"✅ MyMetalView initialization complete with new architecture");
    NSLog(@"📦 Using clean architecture with Application framework");
    NSLog(@"🎮 Controls: WASD=move, Space=up, Q=down, R=reset, Right-click+drag=look");

    return self;
}

- (void)setupRenderTimer:(double)targetFPS {
    uint64_t intervalNanos = (uint64_t)(1.0 / targetFPS * NSEC_PER_SEC);

    dispatch_queue_t queue = dispatch_queue_create("com.metal.renderqueue", DISPATCH_QUEUE_SERIAL);
    _renderTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

    dispatch_source_set_timer(_renderTimer,
                             dispatch_time(DISPATCH_TIME_NOW, 0),
                             intervalNanos,
                             intervalNanos / 10);

    __weak typeof(self) weakSelf = self;
    dispatch_source_set_event_handler(_renderTimer, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf renderFrame];
        });
    });

    dispatch_resume(_renderTimer);
}

- (void)renderFrame {
    // Update time system
    Engine::Core::Time::getInstance().update();
    float deltaTime = Engine::Core::Time::getInstance().getDeltaTime();

    // Update application
    _application->onUpdate(deltaTime);

    // Render with the camera from application
    Camera* camera = _application->getCamera();
    if (camera) {
        [_renderer draw:deltaTime withCamera:*camera];
    }

    // Call application render (for future use)
    _application->onRender();

    // Debug output every 60 frames
    static int frameCounter = 0;
    if (++frameCounter >= 60) {
        frameCounter = 0;
        float fps = Engine::Core::Time::getInstance().getFPS();
        auto& inputState = _application->getInputState();

        NSLog(@"📊 FPS: %.1f | Frame: %llu | Time: %.2f",
              fps,
              Engine::Core::Time::getInstance().getFrameCount(),
              Engine::Core::Time::getInstance().getTime());

        if (inputState.moveForward || inputState.moveBackward ||
            inputState.moveLeft || inputState.moveRight) {
            vector_float3 pos = camera->getPosition();
            NSLog(@"📍 Camera pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        }
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
    return [super becomeFirstResponder];
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        [self.window makeFirstResponder:self];
        NSLog(@"🖼️ View added to window, set as first responder");
    }
}

// ========================================
// Input Handling - Delegated to Application
// ========================================

- (void)keyDown:(NSEvent *)event {
    _application->processKeyDown(event.keyCode);
}

- (void)keyUp:(NSEvent *)event {
    _application->processKeyUp(event.keyCode);
}

- (void)mouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];

    if (event.buttonNumber == 1) { // Right mouse button
        _application->setMouseLook(YES);
        _firstMouseMove = YES;
        _lastMouseLocation = [self convertPoint:event.locationInWindow fromView:nil];
        NSLog(@"🖱️ Mouse look enabled");
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (event.buttonNumber == 1) { // Right mouse button
        _application->setMouseLook(NO);
        NSLog(@"🖱️ Mouse look disabled");
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (_application->getInputState().mouseLookEnabled && event.buttonNumber == 1) {
        NSPoint currentLocation = [self convertPoint:event.locationInWindow fromView:nil];

        if (!_firstMouseMove) {
            float deltaX = currentLocation.x - _lastMouseLocation.x;
            float deltaY = currentLocation.y - _lastMouseLocation.y;

            _application->processMouseMove(deltaX, deltaY);
        } else {
            _firstMouseMove = NO;
        }

        _lastMouseLocation = currentLocation;
    }
}

- (void)rightMouseDown:(NSEvent *)event {
    [self mouseDown:event];
}

- (void)rightMouseUp:(NSEvent *)event {
    [self mouseUp:event];
}

- (void)rightMouseDragged:(NSEvent *)event {
    [self mouseDragged:event];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];

    CGSize newSize = CGSizeMake(self.bounds.size.width * self.window.backingScaleFactor,
                                self.bounds.size.height * self.window.backingScaleFactor);
    _metalLayer.drawableSize = newSize;

    // Notify application of resize
    _application->onResize(newSize.width, newSize.height);
}

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

    [_renderer cleanup];
    _renderer = nil;
    _metalLayer = nil;
    _device = nil;

    NSLog(@"🧹 MyMetalView cleaned up");
}

- (void)dealloc {
    [self cleanup];
}

@end