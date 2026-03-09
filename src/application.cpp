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
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
	var p = vec2f(0.0, 0.0);
	if (in_vertex_index == 0u) {
		p = vec2f(-0.5, -0.5);
	} else if (in_vertex_index == 1u) {
		p = vec2f(0.5, -0.5);
	} else {
		p = vec2f(0.0, 0.5);
	}
	return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 1.0, 1.0);
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
    deviceDesc.requiredLimits           = nullptr;
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

    InitializePipeline();

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
    renderPassColorAttachment.clearValue                = WGPUColor{0.9, 0.1, 0.2, 1.0};
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
    // Draw 1 instance of a 3-vertices shape
    renderPass->draw(3, 1, 0, 0);
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

    // We do not use any vertex buffer for this first simplistic example
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers     = nullptr;

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
