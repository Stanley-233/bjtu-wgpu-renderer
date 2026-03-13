//
// Created by Stanley on 2026/3/6.
//

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <iostream>

#include "application.h"

#include "utils.h"

using namespace wgpu;

// We embed the source of the shader module here
const char* shaderSource = R"(
struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let ratio = 640.0 / 480.0;
    out.position = vec4f(in.position.x, in.position.y * ratio, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}
)";

Application& Application::SetWindowSize(int width, int height) {
    m_windowWidth  = width;
    m_windowHeight = height;
    return *this;
}

bool Application::Initialize() {
    // Open window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "WebGPU Renderer", nullptr, nullptr);

    raii::Instance instance = Instance(wgpuCreateInstance(nullptr));

    std::cout << "Requesting adapter..." << std::endl;
    m_surface                          = Surface(glfwGetWGPUSurface(*instance, m_window));
    RequestAdapterOptions adapterOpts = {};
    adapterOpts.compatibleSurface     = *m_surface;
    raii::Adapter adapter              = instance->requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;
    DeviceDescriptor deviceDesc         = {};
    deviceDesc.label                    = "My Device";
    deviceDesc.requiredFeatureCount     = 0;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label       = "The default queue";
    deviceDesc.deviceLostCallback       = [](WGPUDeviceLostReason reason,
                                        char const*                message,
                                        void* /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    RequiredLimits requiredLimits = GetRequiredLimits(*adapter);
    deviceDesc.requiredLimits = &requiredLimits;
    m_device = adapter->requestDevice(deviceDesc);
    std::cout << "Got device: " << m_device << std::endl;

    m_uncapturedErrorCallbackHandle = m_device->setUncapturedErrorCallback(
        [](ErrorType type, char const* message) {
            std::cout << "Uncaptured device error: type " << type;
            if (message)
                std::cout << " (" << message << ")";
            std::cout << std::endl;
        });

    m_queue = m_device->getQueue();

    // Configure the surface
    SurfaceConfiguration config = {};

    // Configuration of the textures created for the underlying swap chain
    config.width     = m_windowWidth;
    config.height    = m_windowHeight;
    config.usage     = TextureUsage::RenderAttachment;
    m_surfaceFormat = m_surface->getPreferredFormat(*adapter);
    config.format    = m_surfaceFormat;

    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats     = nullptr;
    config.device          = *m_device;
    config.presentMode     = PresentMode::Fifo;
    config.alphaMode       = CompositeAlphaMode::Auto;

    m_surface->configure(config);

    SupportedLimits supportedLimits;
    adapter->getLimits(&supportedLimits);
    std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
    m_device->getLimits(&supportedLimits);
    std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    InitializePipeline();
    InitializeBuffers();

    TestBuffers();

    return true;
}

void Application::Terminate() {
    m_surface->unconfigure();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Application::IsRunning() const {
    return !glfwWindowShouldClose(m_window);
}

void Application::MainLoop() {
    glfwPollEvents();

    // Get the next target texture view
    raii::TextureView targetView = GetNextSurfaceTextureView();
    if (!targetView)
        return;

    // Create a command encoder for the draw call
    CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label                    = "My command encoder";
    raii::CommandEncoder encoder          = CommandEncoder(
        wgpuDeviceCreateCommandEncoder(*m_device, &encoderDesc));

    // Create the render pass that clears the screen with our color
    RenderPassDescriptor renderPassDesc = {};

    // The attachment part of the render pass descriptor describes the target texture of the pass
    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view                      = *targetView;
    renderPassColorAttachment.resolveTarget             = nullptr;
    renderPassColorAttachment.loadOp                    = LoadOp::Clear;
    renderPassColorAttachment.storeOp                   = StoreOp::Store;
    renderPassColorAttachment.clearValue                = WGPUColor{0.05, 0.05, 0.05, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    renderPassDesc.colorAttachmentCount   = 1;
    renderPassDesc.colorAttachments       = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites        = nullptr;

    // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
    raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    // Select which render pipeline to use
    renderPass->setPipeline(*m_pipeline);

    // Set vertex buffer while encoding the render pass
    renderPass->setVertexBuffer(0, *m_pointBuffer, 0, m_pointBuffer->getSize());
    renderPass->setIndexBuffer(*m_indexBuffer, IndexFormat::Uint16, 0, m_indexBuffer->getSize());

    // We use the `vertexCount` variable instead of hard-coding the vertex count
    renderPass->drawIndexed(m_indexCount, 1, 0, 0, 0);

    renderPass->end();

    // Finally encode and submit the render pass
    CommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.label                   = "Command buffer";
    raii::CommandBuffer commandBuffer            = encoder->finish(cmdBufferDescriptor);

    if (enableMainLoopDebug) {
        std::cout << "Submitting command..." << std::endl;
    }
    m_queue->submit(1, commandBuffer.ptr());
    if (enableMainLoopDebug) {
        std::cout << "Command submitted." << std::endl;
    }

    // At the end of the frame
#ifndef __EMSCRIPTEN__
    m_surface->present();
#endif

#ifdef WEBGPU_BACKEND_DAWN
    m_device->tick();
#elif WEBGPU_BACKEND_WGPU
    m_device->poll(false);
#endif
}

raii::TextureView Application::GetNextSurfaceTextureView() {
    SurfaceTexture surfaceTexture;
    m_surface->getCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
        return {};
    }
    Texture texture = surfaceTexture.texture;

    // Create a view for this surface texture
    TextureViewDescriptor viewDescriptor;
    viewDescriptor.label           = "Surface texture view";
    viewDescriptor.format          = texture.getFormat();
    viewDescriptor.dimension       = TextureViewDimension::_2D;
    viewDescriptor.baseMipLevel    = 0;
    viewDescriptor.mipLevelCount   = 1;
    viewDescriptor.baseArrayLayer  = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect          = TextureAspect::All;
    raii::TextureView targetView   = texture.createView(viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
    // We no longer need the texture, only its view
    // (NB: with wgpu-native, surface textures must not be manually released)
    wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU

    return targetView;
}

RequiredLimits Application::GetRequiredLimits(Adapter adapter) {
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    // Don't forget to = Default
    RequiredLimits requiredLimits = Default;

    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 2;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    // Maximum size of a buffer is 6 vertices of 2 float each
    requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);

    // These two limits are different because they are "minimum" limits,
    // they are the only ones we are may forward from the adapter's supported limits.
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

#ifndef __EMSCRIPTEN__
    // There is a maximum of 3 float forwarded from vertex to fragment shader
    requiredLimits.limits.maxInterStageShaderComponents = 3;
#endif

    return requiredLimits;
}

void Application::InitializePipeline() {
    // Load the shader module
    ShaderModuleDescriptor shaderDesc;
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints     = nullptr;
#endif

    // We use the extension mechanism to specify the WGSL part of the shader module descriptor
    ShaderModuleWGSLDescriptor shaderCodeDesc;
    // Set the chained struct's header
    shaderCodeDesc.chain.next  = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain    = &shaderCodeDesc.chain;
    shaderCodeDesc.code      = shaderSource;
    ShaderModule shaderModule = m_device->createShaderModule(shaderDesc);

    // Create the render pipeline
    RenderPipelineDescriptor pipelineDesc;

    // Configure the vertex pipeline
    // We use one vertex buffer
    VertexBufferLayout vertexBufferLayout;
    VertexAttribute positionAttrib;
    // == For each attribute, describe its layout, i.e., how to interpret the raw data ==
    // Corresponds to @location(...)
    positionAttrib.shaderLocation = 0;
    // Means vec2f in the shader
    positionAttrib.format = VertexFormat::Float32x2;
    // Index of the first element
    positionAttrib.offset = 0;

    // We now have 2 attributes
    std::vector<VertexAttribute> vertexAttribs(2);

    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0; // @location(0)
    vertexAttribs[0].format = VertexFormat::Float32x2;
    vertexAttribs[0].offset = 0;
    // Describe the color attribute
    vertexAttribs[1].shaderLocation = 1; // @location(1)
    vertexAttribs[1].format = VertexFormat::Float32x3; // different type!
    vertexAttribs[1].offset = 2 * sizeof(float); // non null offset!

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = 5 * sizeof(float);
    vertexBufferLayout.stepMode = VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;

    // NB: We define the 'shaderModule' in the second part of this chapter.
    // Here we tell that the programmable vertex shader stage is described
    // by the function called 'vs_main' in that module.
    pipelineDesc.vertex.module        = shaderModule;
    pipelineDesc.vertex.entryPoint    = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants     = nullptr;

    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;

    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;

    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = FrontFace::CCW;

    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = CullMode::None;

    // We tell that the programmable fragment shader stage is described
    // by the function called 'fs_main' in the shader module.
    FragmentState fragmentState;
    fragmentState.module        = shaderModule;
    fragmentState.entryPoint    = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants     = nullptr;

    BlendState blendState;
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget;
    colorTarget.format    = m_surfaceFormat;
    colorTarget.blend     = &blendState;
    colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets     = &colorTarget;
    pipelineDesc.fragment     = &fragmentState;

    // We do not use stencil/depth testing for now
    pipelineDesc.depthStencil = nullptr;

    // Samples per pixel
    pipelineDesc.multisample.count = 1;

    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;

    // Default value as well (irrelevant for count = 1 anyway)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
    pipelineDesc.layout                             = nullptr;

    m_pipeline = m_device->createRenderPipeline(pipelineDesc);

    // We no longer need to access the shader module
    shaderModule.release();
}

void Application::InitializeBuffers() {
    // Define point data
    // The de-duplicated list of point positions
    std::vector<float> pointData = {
        // x,   y,     r,   g,   b
        -0.5, -0.5,   1.0, 0.0, 0.0, // Point #0
        +0.5, -0.5,   0.0, 1.0, 0.0, // Point #1
        +0.5, +0.5,   0.0, 0.0, 1.0, // Point #2
        -0.5, +0.5,   1.0, 1.0, 0.0  // Point #3
    };

    // Define index data
    // This is a list of indices referencing positions in the pointData
    std::vector<uint16_t> indexData = {
        0, 1, 2, // Triangle #0 connects points #0, #1 and #2
        0, 2, 3  // Triangle #1 connects points #0, #2 and #3
    };

    m_indexCount = static_cast<uint32_t>(indexData.size());

    // Create point buffer
    BufferDescriptor bufferDesc;
    bufferDesc.size = pointData.size() * sizeof(float);
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex; // Vertex usage here!
    bufferDesc.mappedAtCreation = false;
    m_pointBuffer = m_device->createBuffer(bufferDesc);
    m_queue->writeBuffer(*m_pointBuffer, 0, pointData.data(), bufferDesc.size);

    // Create index buffer
    bufferDesc.size = indexData.size() * sizeof(uint16_t);
    bufferDesc.size = (bufferDesc.size + 3) & ~3; // round up to the next multiple of 4
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    m_indexBuffer = m_device->createBuffer(bufferDesc);

    m_queue->writeBuffer(*m_indexBuffer, 0, indexData.data(), bufferDesc.size);
}

void Application::TestBuffers() {
    BufferDescriptor bufferDesc;
    bufferDesc.label = "Some GPU-side data buffer";
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::CopySrc;
    bufferDesc.size = 16;
    bufferDesc.mappedAtCreation = false;
    raii::Buffer buffer1 = m_device->createBuffer(bufferDesc);
    bufferDesc.label = "Output buffer";
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::MapRead;
    raii::Buffer outputBuffer = m_device->createBuffer(bufferDesc);

    // Create some CPU-side data buffer (of size 16 bytes)
    std::vector<uint8_t> numbers(16);
    for (uint8_t i = 0; i < 16; ++i) {
        numbers[i] = i;
    }
    // Copy this from `numbers` (RAM) to `buffer1` (VRAM)
    m_queue->writeBuffer(*buffer1, 0, numbers.data(), numbers.size());
    raii::CommandEncoder encoder = m_device->createCommandEncoder(Default);
    encoder->copyBufferToBuffer(*buffer1, 0, *outputBuffer, 0, numbers.size());
    raii::CommandBuffer cmdBuffer = encoder->finish(Default);
    m_queue->submit(1, cmdBuffer.ptr());

    bool ready = false;
    auto onOutputBufferMapped =
        outputBuffer->mapAsync(MapMode::Read,0, 16,
            [&ready, &outputBuffer](BufferMapAsyncStatus status) -> void {
        ready = true;
        std::cout << "Buffer 2 mapped with status " << status << std::endl;
        if (status != BufferMapAsyncStatus::Success) return;
        uint8_t* bufferData = (uint8_t*)outputBuffer->getConstMappedRange(0, 16);
        std::cout << "bufferData = [";
        for (int i = 0; i < 16; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << static_cast<int>(bufferData[i]);
        }
        std::cout << "]" << std::endl;
        outputBuffer->unmap();
    });

    while (!ready) {
        wgpuPollEvents(*m_device, true);
    }
}
