#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>
#include "Camera.hpp"

@interface Renderer : NSObject
- (instancetype)initWithDevice:(id<MTLDevice>)dev layer:(CAMetalLayer *)layer;
- (void)draw:(double)dt withCamera:(Camera&)camera;
- (void)cleanup;
@end