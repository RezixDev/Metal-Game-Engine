// MetalRenderer.h
#pragma once

#import <MetalKit/MetalKit.h>

@interface MetalRenderer : NSObject <MTKViewDelegate>

- (instancetype)initWithMTKView:(MTKView *)view;

// Input — driven by the host view.
- (void)keyDown:(unsigned short)keyCode;
- (void)keyUp:(unsigned short)keyCode;
- (void)setMouseLookEnabled:(BOOL)enabled;
- (void)processMouseDeltaX:(float)dx deltaY:(float)dy;

// Particle sandbox — cursor tracking for particle flocking.
- (void)setCursorScreenPosition:(CGPoint)pt viewSize:(CGSize)viewSize;

@end
