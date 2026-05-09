import re

with open("MetalEngine/MetalRenderer.mm", "r") as f:
    content = f.read()

# 1. Imports
content = content.replace('#include "Engine/Time.hpp"\n', '#include "Engine/Time.hpp"\n#import <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>\n')

# 2. Ivars
ivars_old = """    // ---- Particle Sandbox ----
    id<MTLBuffer>               _particleBuffer;
    id<MTLBuffer>               _brainBuffer;
    id<MTLBuffer>               _predatorBuffer;
    id<MTLComputePipelineState> _computePipelineParticles;
    id<MTLComputePipelineState> _computePipelinePredators;
    id<MTLRenderPipelineState>  _particleRenderPipeline;"""

ivars_new = """    // ---- Particle Sandbox ----
    id<MTLBuffer>               _particleBuffer;
    id<MTLBuffer>               _predatorBuffer;
    id<MTLComputePipelineState> _computePipelinePrepare;
    id<MTLComputePipelineState> _computePipelinePhysics;
    id<MTLComputePipelineState> _computePipelinePredators;
    id<MTLRenderPipelineState>  _particleRenderPipeline;

    // ---- MPSGraph ----
    MPSGraph                   *_brainGraph;
    MPSGraphExecutable         *_brainExecutable;
    id<MTLBuffer>               _featureBuffer;
    id<MTLBuffer>               _accelBuffer;
    MPSGraphTensor             *_inputTensor;
    MPSGraphTensor             *_outputTensor;"""
content = content.replace(ivars_old, ivars_new)

# 3. Pipelines
pipelines_old = """    id<MTLFunction> fnParticles = [lib newFunctionWithName:@"updateParticles"];
    _computePipelineParticles = [_device newComputePipelineStateWithFunction:fnParticles
                                                                      error:&error];
    if (!_computePipelineParticles) {
        [NSException raise:@"MetalRenderer"
                    format:@"Failed to create particle compute pipeline: %@", error];
    }"""

pipelines_new = """    id<MTLFunction> fnPrepare = [lib newFunctionWithName:@"prepareFeatures"];
    _computePipelinePrepare = [_device newComputePipelineStateWithFunction:fnPrepare error:&error];
    if (!_computePipelinePrepare) [NSException raise:@"MetalRenderer" format:@"Failed to create prepareFeatures pipeline: %@", error];

    id<MTLFunction> fnPhysics = [lib newFunctionWithName:@"applyPhysics"];
    _computePipelinePhysics = [_device newComputePipelineStateWithFunction:fnPhysics error:&error];
    if (!_computePipelinePhysics) [NSException raise:@"MetalRenderer" format:@"Failed to create applyPhysics pipeline: %@", error];"""
content = content.replace(pipelines_old, pipelines_new)

# 4. Particle init
init_old = """    // ---- Brains: identity-like weights that gently steer toward cursor ----
    {
        std::vector<Engine::Brain> brains(_particleCount);
        for (NSUInteger i = 0; i < _particleCount; ++i) {
            // Hidden layer: pass through with slight randomisation
            float r1 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r2 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r3 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r4 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;

            brains[i].w1 = (matrix_float4x4){{
                {1.0f + r1, 0, 0, 0},
                {0, 1.0f + r2, 0, 0},
                {0, 0, 1.0f + r3, 0},
                {0, 0, 0, 1.0f + r4},
            }};
            brains[i].b1 = simd_make_float4(0, 0, 0, 0);

            // Output layer: map hidden[0,1] (cursor dx, dz) → acceleration
            // Strong gain so particles visibly accelerate toward cursor
            float gain = 0.6f + (rand() / (float)RAND_MAX) * 0.4f;
            float cross = (rand() / (float)RAND_MAX - 0.5f) * 0.05f;
            brains[i].w2 = (matrix_float4x4){{
                {gain,  cross, 0, 0},
                {cross, gain,  0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            }};
            brains[i].b2 = simd_make_float4(0, 0, 0, 0);
        }
        _brainBuffer = [_device newBufferWithBytes:brains.data()
                                            length:brains.size() * sizeof(Engine::Brain)
                                           options:MTLResourceStorageModeShared];
        _brainBuffer.label = @"BrainBuffer";
    }"""

init_new = """    // ---- MPSGraph Buffers ----
    NSUInteger featureBytes = _particleCount * Engine::kInputSize * sizeof(float);
    _featureBuffer = [_device newBufferWithLength:featureBytes options:MTLResourceStorageModePrivate];
    _featureBuffer.label = @"FeatureBuffer";
    
    NSUInteger accelBytes = _particleCount * Engine::kOutputSize * sizeof(float);
    _accelBuffer = [_device newBufferWithLength:accelBytes options:MTLResourceStorageModePrivate];
    _accelBuffer.label = @"AccelBuffer";

    // Build the brain
    [self buildBrainGraphWithFactor:1.0f];"""
content = content.replace(init_old, init_new)

# 5. Build Graph & Mutate method
build_graph = """

- (void)buildBrainGraphWithFactor:(float)factor {
    _brainGraph = [[MPSGraph alloc] init];
    
    // Input: [N, 4]
    _inputTensor = [_brainGraph placeholderWithShape:@[@(Engine::kParticleCount), @(Engine::kInputSize)]
                                            dataType:MPSDataTypeFloat32
                                                name:@"features"];
    
    // Hidden layer weights [4, 16]
    float w1[Engine::kInputSize * Engine::kHiddenSize];
    for (int i = 0; i < Engine::kInputSize * Engine::kHiddenSize; ++i) {
        w1[i] = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
    }
    // Boost cursor x/y weights to ensure response
    w1[0] = 1.0f; w1[1] = 0.0f; w1[2] = 0.0f; w1[3] = 0.0f; // x -> h[0]
    w1[1*Engine::kHiddenSize + 1] = 1.0f;                   // y -> h[1]

    NSData *w1Data = [NSData dataWithBytes:w1 length:sizeof(w1)];
    MPSGraphTensor *w1T = [_brainGraph constantWithData:w1Data
                                                  shape:@[@(Engine::kInputSize), @(Engine::kHiddenSize)]
                                               dataType:MPSDataTypeFloat32];
    
    float b1[Engine::kHiddenSize] = {0};
    NSData *b1Data = [NSData dataWithBytes:b1 length:sizeof(b1)];
    MPSGraphTensor *b1T = [_brainGraph constantWithData:b1Data
                                                  shape:@[@(1), @(Engine::kHiddenSize)]
                                               dataType:MPSDataTypeFloat32];
    
    MPSGraphTensor *h = [_brainGraph matrixMultiplicationWithPrimaryTensor:_inputTensor secondaryTensor:w1T name:nil];
    h = [_brainGraph additionWithPrimaryTensor:h secondaryTensor:b1T name:nil];
    h = [_brainGraph reLUWithTensor:h name:@"hidden"];
    
    // Output layer weights [16, 2]
    float w2[Engine::kHiddenSize * Engine::kOutputSize] = {0};
    // Map h[0] -> accel_x, h[1] -> accel_y
    w2[0] = factor; // h[0] -> out[0]
    w2[1*Engine::kOutputSize + 1] = factor; // h[1] -> out[1]
    
    NSData *w2Data = [NSData dataWithBytes:w2 length:sizeof(w2)];
    MPSGraphTensor *w2T = [_brainGraph constantWithData:w2Data
                                                  shape:@[@(Engine::kHiddenSize), @(Engine::kOutputSize)]
                                               dataType:MPSDataTypeFloat32];
    
    float b2[Engine::kOutputSize] = {0};
    NSData *b2Data = [NSData dataWithBytes:b2 length:sizeof(b2)];
    MPSGraphTensor *b2T = [_brainGraph constantWithData:b2Data
                                                  shape:@[@(1), @(Engine::kOutputSize)]
                                               dataType:MPSDataTypeFloat32];
    
    MPSGraphTensor *out = [_brainGraph matrixMultiplicationWithPrimaryTensor:h secondaryTensor:w2T name:nil];
    _outputTensor = [_brainGraph additionWithPrimaryTensor:out secondaryTensor:b2T name:@"acceleration"];
    
    _brainExecutable = [_brainGraph compileWithDevice:_device
                                          feeds:@{ _inputTensor: [MPSGraphShapedType shapedTypeWithShape:@[@(Engine::kParticleCount), @(Engine::kInputSize)] dataType:MPSDataTypeFloat32] }
                                  targetTensors:@[_outputTensor]
                               targetOperations:nil
                            compilationDescriptor:nil];
}

- (void)mutateBrain:(float)factor {
    [self buildBrainGraphWithFactor:factor];
}

- (void)addMesh:(const Engine::MeshData&)data
"""
content = content.replace("\n- (void)addMesh:(const Engine::MeshData&)data\n", build_graph)

# 6. Render loop compute dispatch
render_old = """        // ==== Compute: Update Particles (Neural Network + Physics) ====
        {
            id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
            [compEnc setComputePipelineState:_computePipelineParticles];
            [compEnc setBuffer:_particleBuffer  offset:0 atIndex:0];
            [compEnc setBuffer:_brainBuffer     offset:0 atIndex:1];
            [compEnc setBuffer:_predatorBuffer  offset:0 atIndex:2];
            uint32_t predCount = Engine::kPredatorCount;
            [compEnc setBytes:&predCount      length:sizeof(uint32_t)    atIndex:3];
            [compEnc setBytes:&_cursorWorld    length:sizeof(simd_float2) atIndex:4];
            float dtCopy = dt;
            [compEnc setBytes:&dtCopy         length:sizeof(float)       atIndex:5];
            float maxSpeed = 10.0f;
            [compEnc setBytes:&maxSpeed       length:sizeof(float)       atIndex:6];

            NSUInteger threadgroupSize = _computePipelineParticles.maxTotalThreadsPerThreadgroup;
            if (threadgroupSize > 256) threadgroupSize = 256;
            MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
            MTLSize groups  = MTLSizeMake(threadgroupSize, 1, 1);
            [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
            [compEnc endEncoding];
        }"""

render_new = """        // ==== Compute: Prepare Features ====
        {
            id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
            [compEnc setComputePipelineState:_computePipelinePrepare];
            [compEnc setBuffer:_particleBuffer offset:0 atIndex:0];
            [compEnc setBuffer:_featureBuffer offset:0 atIndex:1];
            [compEnc setBytes:&_cursorWorld length:sizeof(simd_float2) atIndex:2];
            float dtCopy = dt;
            [compEnc setBytes:&dtCopy length:sizeof(float) atIndex:3];
            uint32_t pCount = _particleCount;
            [compEnc setBytes:&pCount length:sizeof(uint32_t) atIndex:4];

            NSUInteger threadgroupSize = _computePipelinePrepare.maxTotalThreadsPerThreadgroup;
            if (threadgroupSize > 256) threadgroupSize = 256;
            MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
            MTLSize groups  = MTLSizeMake(threadgroupSize, 1, 1);
            [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
            [compEnc endEncoding];
        }

        // ==== MPSGraph: Inference ====
        MPSGraphTensorData *inputData = [[MPSGraphTensorData alloc] initWithMTLBuffer:_featureBuffer
                                                                                shape:@[@(_particleCount), @(Engine::kInputSize)]
                                                                             dataType:MPSDataTypeFloat32];
        MPSGraphTensorData *outputData = [[MPSGraphTensorData alloc] initWithMTLBuffer:_accelBuffer
                                                                                 shape:@[@(_particleCount), @(Engine::kOutputSize)]
                                                                              dataType:MPSDataTypeFloat32];

        [_brainExecutable encodeToCommandBuffer:cmd
                                     inputsArray:@[inputData]
                                    resultsArray:@[outputData]
                             executionDescriptor:nil];

        // ==== Compute: Apply Physics ====
        {
            id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
            [compEnc setComputePipelineState:_computePipelinePhysics];
            [compEnc setBuffer:_particleBuffer offset:0 atIndex:0];
            [compEnc setBuffer:_accelBuffer offset:0 atIndex:1];
            [compEnc setBuffer:_predatorBuffer offset:0 atIndex:2];
            uint32_t predCount = Engine::kPredatorCount;
            [compEnc setBytes:&predCount length:sizeof(uint32_t) atIndex:3];
            float dtCopy = dt;
            [compEnc setBytes:&dtCopy length:sizeof(float) atIndex:4];
            float maxSpeed = 10.0f;
            [compEnc setBytes:&maxSpeed length:sizeof(float) atIndex:5];
            uint32_t pCount = _particleCount;
            [compEnc setBytes:&pCount length:sizeof(uint32_t) atIndex:6];

            NSUInteger threadgroupSize = _computePipelinePhysics.maxTotalThreadsPerThreadgroup;
            if (threadgroupSize > 256) threadgroupSize = 256;
            MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
            MTLSize groups  = MTLSizeMake(threadgroupSize, 1, 1);
            [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
            [compEnc endEncoding];
        }"""
content = content.replace(render_old, render_new)

with open("MetalEngine/MetalRenderer.mm", "w") as f:
    f.write(content)
