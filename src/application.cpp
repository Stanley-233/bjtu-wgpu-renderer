//
// Created by Stanley on 2026/3/6.
//

#include <GLFW/glfw3.h>
#include "../ext/glfw3webgpu/glfw3webgpu.h"

#include <iostream>

#include "application.h"

using namespace wgpu;

// We embed the source of the shader module here
const char* shader_source = R"(
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
    m_window_width  = width;
    m_window_height = height;
    return *this;
}

bool Application::Initialize() {
    // Open window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_window_width, m_window_height, "WebGPU Renderer", nullptr, nullptr);

    raii::Instance instance = Instance(wgpuCreateInstance(nullptr));

    std::cout << "Requesting adapter..." << std::endl;
    m_surface                          = Surface(glfwGetWGPUSurface(*instance, m_window));
    RequestAdapterOptions adapter_opts = {};
    adapter_opts.compatibleSurface     = *m_surface;
    raii::Adapter adapter              = instance->requestAdapter(adapter_opts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;
    DeviceDescriptor device_desc         = {};
    device_desc.label                    = "My Device";
    device_desc.requiredFeatureCount     = 0;
    device_desc.requiredLimits           = nullptr;
    device_desc.defaultQueue.nextInChain = nullptr;
    device_desc.defaultQueue.label       = "The default queue";
    device_desc.deviceLostCallback       = [](WGPUDeviceLostReason reason,
                                        char const*                message,
                                        void* /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    m_device = adapter->requestDevice(device_desc);
    std::cout << "Got device: " << m_device << std::endl;

    m_uncaptured_error_callback_handle = m_device->setUncapturedErrorCallback(
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
    config.width     = m_window_width;
    config.height    = m_window_height;
    config.usage     = TextureUsage::RenderAttachment;
    m_surface_format = m_surface->getPreferredFormat(*adapter);
    config.format    = m_surface_format;

    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats     = nullptr;
    config.device          = *m_device;
    config.presentMode     = PresentMode::Fifo;
    config.alphaMode       = CompositeAlphaMode::Auto;

    m_surface->configure(config);

    InitializePipeline();

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
    raii::TextureView target_view = GetNextSurfaceTextureView();
    if (!target_view)
        return;

    // Create a command encoder for the draw call
    CommandEncoderDescriptor encoder_desc = {};
    encoder_desc.label                    = "My command encoder";
    raii::CommandEncoder encoder          = CommandEncoder(
        wgpuDeviceCreateCommandEncoder(*m_device, &encoder_desc));

    // Create the render pass that clears the screen with our color
    RenderPassDescriptor render_pass_desc = {};

    // The attachment part of the render pass descriptor describes the target texture of the pass
    RenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view                      = *target_view;
    render_pass_color_attachment.resolveTarget             = nullptr;
    render_pass_color_attachment.loadOp                    = LoadOp::Clear;
    render_pass_color_attachment.storeOp                   = StoreOp::Store;
    render_pass_color_attachment.clearValue                = WGPUColor{0.9, 0.1, 0.2, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    render_pass_desc.colorAttachmentCount   = 1;
    render_pass_desc.colorAttachments       = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = nullptr;
    render_pass_desc.timestampWrites        = nullptr;

    // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
    raii::RenderPassEncoder render_pass = encoder->beginRenderPass(render_pass_desc);
    // Select which render pipeline to use
    render_pass->setPipeline(*m_pipeline);
    // Draw 1 instance of a 3-vertices shape
    render_pass->draw(3, 1, 0, 0);
    render_pass->end();

    // Finally encode and submit the render pass
    CommandBufferDescriptor cmd_buffer_descriptor = {};
    cmd_buffer_descriptor.label                   = "Command buffer";
    raii::CommandBuffer command_buffer            = encoder->finish(cmd_buffer_descriptor);

    if (enable_main_loop_debug) {
        std::cout << "Submitting command..." << std::endl;
    }
    m_queue->submit(1, command_buffer.ptr());
    if (enable_main_loop_debug) {
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
    SurfaceTexture surface_texture;
    m_surface->getCurrentTexture(&surface_texture);
    if (surface_texture.status != SurfaceGetCurrentTextureStatus::Success) {
        return {};
    }
    Texture texture = surface_texture.texture;

    // Create a view for this surface texture
    TextureViewDescriptor view_descriptor;
    view_descriptor.label           = "Surface texture view";
    view_descriptor.format          = texture.getFormat();
    view_descriptor.dimension       = TextureViewDimension::_2D;
    view_descriptor.baseMipLevel    = 0;
    view_descriptor.mipLevelCount   = 1;
    view_descriptor.baseArrayLayer  = 0;
    view_descriptor.arrayLayerCount = 1;
    view_descriptor.aspect          = TextureAspect::All;
    raii::TextureView target_view   = texture.createView(view_descriptor);

#ifndef WEBGPU_BACKEND_WGPU
    // We no longer need the texture, only its view
    // (NB: with wgpu-native, surface textures must not be manually released)
    wgpuTextureRelease(surface_texture.texture);
#endif // WEBGPU_BACKEND_WGPU

    return target_view;
}

void Application::InitializePipeline() {
    // Load the shader module
    ShaderModuleDescriptor shader_desc;
#ifdef WEBGPU_BACKEND_WGPU
    shader_desc.hintCount = 0;
    shader_desc.hints     = nullptr;
#endif

    // We use the extension mechanism to specify the WGSL part of the shader module descriptor
    ShaderModuleWGSLDescriptor shader_code_desc;
    // Set the chained struct's header
    shader_code_desc.chain.next  = nullptr;
    shader_code_desc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    // Connect the chain
    shader_desc.nextInChain    = &shader_code_desc.chain;
    shader_code_desc.code      = shader_source;
    ShaderModule shader_module = m_device->createShaderModule(shader_desc);

    // Create the render pipeline
    RenderPipelineDescriptor pipeline_desc;

    // We do not use any vertex buffer for this first simplistic example
    pipeline_desc.vertex.bufferCount = 0;
    pipeline_desc.vertex.buffers     = nullptr;

    // NB: We define the 'shaderModule' in the second part of this chapter.
    // Here we tell that the programmable vertex shader stage is described
    // by the function called 'vs_main' in that module.
    pipeline_desc.vertex.module        = shader_module;
    pipeline_desc.vertex.entryPoint    = "vs_main";
    pipeline_desc.vertex.constantCount = 0;
    pipeline_desc.vertex.constants     = nullptr;

    // Each sequence of 3 vertices is considered as a triangle
    pipeline_desc.primitive.topology = PrimitiveTopology::TriangleList;

    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipeline_desc.primitive.stripIndexFormat = IndexFormat::Undefined;

    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipeline_desc.primitive.frontFace = FrontFace::CCW;

    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipeline_desc.primitive.cullMode = CullMode::None;

    // We tell that the programmable fragment shader stage is described
    // by the function called 'fs_main' in the shader module.
    FragmentState fragment_state;
    fragment_state.module        = shader_module;
    fragment_state.entryPoint    = "fs_main";
    fragment_state.constantCount = 0;
    fragment_state.constants     = nullptr;

    BlendState blend_state;
    blend_state.color.srcFactor = BlendFactor::SrcAlpha;
    blend_state.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blend_state.color.operation = BlendOperation::Add;
    blend_state.alpha.srcFactor = BlendFactor::Zero;
    blend_state.alpha.dstFactor = BlendFactor::One;
    blend_state.alpha.operation = BlendOperation::Add;

    ColorTargetState color_target;
    color_target.format    = m_surface_format;
    color_target.blend     = &blend_state;
    color_target.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragment_state.targetCount = 1;
    fragment_state.targets     = &color_target;
    pipeline_desc.fragment     = &fragment_state;

    // We do not use stencil/depth testing for now
    pipeline_desc.depthStencil = nullptr;

    // Samples per pixel
    pipeline_desc.multisample.count = 1;

    // Default value for the mask, meaning "all bits on"
    pipeline_desc.multisample.mask = ~0u;

    // Default value as well (irrelevant for count = 1 anyway)
    pipeline_desc.multisample.alphaToCoverageEnabled = false;
    pipeline_desc.layout                             = nullptr;

    m_pipeline = m_device->createRenderPipeline(pipeline_desc);

    // We no longer need to access the shader module
    shader_module.release();
}