//
// Reset MyMetalView.mm - Better camera positioning and movement debugging
//
#import "MyMetalView.h"
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <mach/mach_time.h>
#import "InputManager.hpp"
#import "Renderer.hpp"
#import "Camera.hpp"

@implementation MyMetalView {
  CAMetalLayer* _metalLayer;
  id<MTLDevice> _device;
  Renderer* _renderer;
  dispatch_source_t _renderTimer;
  uint64_t _lastFrameTime;
  mach_timebase_info_data_t _timebaseInfo;
  
  // Camera system
  Camera* _camera;

  // Movement state tracking
  BOOL _movingForward;
  BOOL _movingBackward;
  BOOL _movingLeft;
  BOOL _movingRight;
  BOOL _movingUp;
  BOOL _movingDown;
  
  // Mouse look
  BOOL _mouseLookEnabled;
  NSPoint _lastMouseLocation;
  BOOL _firstMouseMove;
}

+ (Class)layerClass { return [CAMetalLayer class]; }

- (instancetype)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (!self) return self;

  NSLog(@"🚀 Starting MyMetalView initialization...");

  _device = MTLCreateSystemDefaultDevice();
  if (!_device) {
      NSLog(@"❌ Failed to create Metal device");
      return nil;
  }
  NSLog(@"✅ Metal device created: %@", _device.name);
  
  _metalLayer = [CAMetalLayer layer];
  _metalLayer.device = _device;
  _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  _metalLayer.framebufferOnly = YES;
  _metalLayer.contentsScale = [NSScreen mainScreen].backingScaleFactor;
  
  // Enable vsync for smooth rendering
  _metalLayer.displaySyncEnabled = YES;
  
  self.layer = _metalLayer;
  self.wantsLayer = YES;

  _renderer = [[Renderer alloc] initWithDevice:_device layer:_metalLayer];
  if (!_renderer) {
      NSLog(@"❌ Failed to create renderer");
      return nil;
  }

  // OPTIMAL camera setup - positioned to see the cube grid clearly
  vector_float3 cameraPos = {-5.0f, 3.0f, 10.0f};   // Back and slightly up and left
  vector_float3 cameraTarget = {0.0f, 0.0f, 0.0f};  // Looking at center
  vector_float3 cameraUp = {0.0f, 1.0f, 0.0f};      // Standard up vector
  
  _camera = new Camera(cameraPos, cameraTarget, cameraUp);
  _camera->setMovementSpeed(3.0f);  // Reasonable movement speed
  
  NSLog(@"🎯 Camera setup: position(-5, 3, 10) -> target(0, 0, 0)");
  NSLog(@"🎮 Controls: WASD=move, Space=up, Q=down, R=reset, Right-click+drag=look");
  NSLog(@"📦 Multiple small colorful cubes at various positions");

  // Initialize movement state
  _movingForward = NO;
  _movingBackward = NO;
  _movingLeft = NO;
  _movingRight = NO;
  _movingUp = NO;
  _movingDown = NO;
  
  // Initialize mouse look
  _mouseLookEnabled = NO;
  _firstMouseMove = YES;

  mach_timebase_info(&_timebaseInfo);
  _lastFrameTime = 0;

  // Create high-precision timer for 60 FPS
  [self setupRenderTimer:60.0];

  NSLog(@"✅ MyMetalView initialization complete");
  return self;
}

- (void)setupRenderTimer:(double)targetFPS {
    // Calculate timer interval in nanoseconds
    uint64_t intervalNanos = (uint64_t)(1.0 / targetFPS * NSEC_PER_SEC);
    
    // Create dispatch timer
    dispatch_queue_t queue = dispatch_queue_create("com.metal.renderqueue", DISPATCH_QUEUE_SERIAL);
    _renderTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
    
    // Set timer to fire at target FPS
    dispatch_source_set_timer(_renderTimer,
                             dispatch_time(DISPATCH_TIME_NOW, 0),
                             intervalNanos,
                             intervalNanos / 10); // Allow 10% leeway for power efficiency
    
    // Weak reference to avoid retain cycle
    __weak typeof(self) weakSelf = self;
    dispatch_source_set_event_handler(_renderTimer, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf renderFrame];
        });
    });
    
    dispatch_resume(_renderTimer);
}

- (void)renderFrame {
  uint64_t now = mach_absolute_time();
  double dt = _lastFrameTime ? (double)(now - _lastFrameTime) * _timebaseInfo.numer / _timebaseInfo.denom / 1e9 : 0.0;
  _lastFrameTime = now;

  // Cap delta time to prevent huge jumps
  dt = fmin(dt, 1.0/30.0);  // Cap at 30 FPS minimum

  // Process camera movement based on current input state
  _camera->processMovement((float)dt, _movingForward, _movingBackward,
                          _movingLeft, _movingRight, _movingUp, _movingDown);

  // Render with the camera
  [_renderer draw:dt withCamera:*_camera];
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

- (void)mouseDown:(NSEvent *)event {
    // Force focus on mouse click
    [self.window makeFirstResponder:self];
    
    if (event.buttonNumber == 1) { // Right mouse button
        _mouseLookEnabled = YES;
        _firstMouseMove = YES;
        _lastMouseLocation = [self convertPoint:event.locationInWindow fromView:nil];
        NSLog(@"🖱️ Mouse look enabled - drag to look around");
    } else {
        NSLog(@"🖱️ Mouse clicked, grabbing focus");
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (event.buttonNumber == 1) { // Right mouse button
        _mouseLookEnabled = NO;
        NSLog(@"🖱️ Mouse look disabled");
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (_mouseLookEnabled && event.buttonNumber == 1) {
        NSPoint currentLocation = [self convertPoint:event.locationInWindow fromView:nil];
        
        if (!_firstMouseMove) {
            float deltaX = currentLocation.x - _lastMouseLocation.x;
            float deltaY = currentLocation.y - _lastMouseLocation.y;
            
            // Process mouse movement for camera rotation
            _camera->processMouseMovement(deltaX, deltaY, 0.002f);
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

- (void)keyDown:(NSEvent *)event {
    switch(event.keyCode) {
        case 13: // W key
            _movingForward = YES;
            NSLog(@"⬆️ FORWARD START");
            break;
        case 1:  // S key
            _movingBackward = YES;
            NSLog(@"⬇️ BACKWARD START");
            break;
        case 0:  // A key
            _movingLeft = YES;
            NSLog(@"⬅️ LEFT START");
            break;
        case 2:  // D key
            _movingRight = YES;
            NSLog(@"➡️ RIGHT START");
            break;
        case 49: // Space key
            _movingUp = YES;
            NSLog(@"⬆️ UP START");
            break;
        case 12: // Q key (down movement)
            _movingDown = YES;
            NSLog(@"⬇️ DOWN START");
            break;
        case 15: // R key - reset camera
            [self resetCamera];
            NSLog(@"🔄 CAMERA RESET");
            break;
        default:
            NSLog(@"❓ Unknown key: %d", (int)event.keyCode);
            [super keyDown:event];
            break;
    }
}

- (void)keyUp:(NSEvent *)event {
    switch(event.keyCode) {
        case 13: // W key
            _movingForward = NO;
            NSLog(@"⏹️ FORWARD STOP");
            break;
        case 1:  // S key
            _movingBackward = NO;
            NSLog(@"⏹️ BACKWARD STOP");
            break;
        case 0:  // A key
            _movingLeft = NO;
            NSLog(@"⏹️ LEFT STOP");
            break;
        case 2:  // D key
            _movingRight = NO;
            NSLog(@"⏹️ RIGHT STOP");
            break;
        case 49: // Space key
            _movingUp = NO;
            NSLog(@"⏹️ UP STOP");
            break;
        case 12: // Q key
            _movingDown = NO;
            NSLog(@"⏹️ DOWN STOP");
            break;
        default:
            [super keyUp:event];
            break;
    }
}

- (void)resetCamera {
    vector_float3 cameraPos = {-5.0f, 3.0f, 10.0f};
    vector_float3 cameraTarget = {0.0f, 0.0f, 0.0f};
    vector_float3 cameraUp = {0.0f, 1.0f, 0.0f};
    
    _camera->lookAt(cameraPos, cameraTarget, cameraUp);
    NSLog(@"📍 Camera reset to: position(-5, 3, 10) looking at origin");
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];
    _metalLayer.drawableSize = CGSizeMake(self.bounds.size.width * self.window.backingScaleFactor,
                                          self.bounds.size.height * self.window.backingScaleFactor);
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

    if (_camera) {
        delete _camera;
        _camera = nullptr;
    }

    [_renderer cleanup];
    _renderer = nil;
    _metalLayer = nil;
    _device = nil;
}

- (void)dealloc {
    [self cleanup];
}

@end
