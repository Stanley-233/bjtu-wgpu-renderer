//
// Created by Stanley on 2026/3/6.
//

#include <GLFW/glfw3.h>
#include "../ext/glfw3webgpu/glfw3webgpu.h"

#include <iostream>

#include "application.h"

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

    wgpu::raii::Instance instance = wgpu::Instance(wgpuCreateInstance(nullptr));

    std::cout << "Requesting adapter..." << std::endl;
    m_surface                                = wgpu::Surface(glfwGetWGPUSurface(*instance, m_window));
    wgpu::RequestAdapterOptions adapter_opts = {};
    adapter_opts.compatibleSurface           = *m_surface;
    wgpu::raii::Adapter adapter              = instance->requestAdapter(adapter_opts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;
    wgpu::DeviceDescriptor device_desc   = {};
    device_desc.label                    = "My Device";
    device_desc.requiredFeatureCount     = 0;
    device_desc.requiredLimits           = nullptr;
    device_desc.defaultQueue.nextInChain = nullptr;
    device_desc.defaultQueue.label       = "The default queue";
    device_desc.deviceLostCallback       = [](WGPUDeviceLostReason reason,
                                              char const*          message,
                                              void* /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    m_device = adapter->requestDevice(device_desc);
    std::cout << "Got device: " << m_device << std::endl;

    m_uncaptured_error_callback_handle = m_device->setUncapturedErrorCallback(
        [](wgpu::ErrorType type, char const* message) {
            std::cout << "Uncaptured device error: type " << type;
            if (message)
                std::cout << " (" << message << ")";
            std::cout << std::endl;
        });

    m_queue = m_device->getQueue();

    // Configure the surface
    wgpu::SurfaceConfiguration config = {};

    // Configuration of the textures created for the underlying swap chain
    config.width                       = m_window_width;
    config.height                      = m_window_height;
    config.usage                       = wgpu::TextureUsage::RenderAttachment;
    wgpu::TextureFormat surface_format = m_surface->getPreferredFormat(*adapter);
    config.format                      = surface_format;

    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats     = nullptr;
    config.device          = *m_device;
    config.presentMode     = wgpu::PresentMode::Fifo;
    config.alphaMode       = wgpu::CompositeAlphaMode::Auto;

    m_surface->configure(config);

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
    wgpu::raii::TextureView target_view = GetNextSurfaceTextureView();
    if (!target_view)
        return;

    // Create a command encoder for the draw call
    wgpu::CommandEncoderDescriptor encoder_desc = {};
    encoder_desc.label                          = "My command encoder";
    wgpu::raii::CommandEncoder encoder          = wgpu::CommandEncoder(
        wgpuDeviceCreateCommandEncoder(*m_device, &encoder_desc));

    // Create the render pass that clears the screen with our color
    wgpu::RenderPassDescriptor render_pass_desc = {};

    // The attachment part of the render pass descriptor describes the target texture of the pass
    wgpu::RenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view                            = *target_view;
    render_pass_color_attachment.resolveTarget                   = nullptr;
    render_pass_color_attachment.loadOp                          = wgpu::LoadOp::Clear;
    render_pass_color_attachment.storeOp                         = wgpu::StoreOp::Store;
    render_pass_color_attachment.clearValue                      = WGPUColor{0.9, 0.1, 0.2, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    render_pass_desc.colorAttachmentCount   = 1;
    render_pass_desc.colorAttachments       = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = nullptr;
    render_pass_desc.timestampWrites        = nullptr;

    // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
    wgpu::raii::RenderPassEncoder render_pass = encoder->beginRenderPass(render_pass_desc);
    render_pass->end();

    // Finally encode and submit the render pass
    wgpu::CommandBufferDescriptor cmd_buffer_descriptor = {};
    cmd_buffer_descriptor.label                         = "Command buffer";
    wgpu::raii::CommandBuffer command_buffer            = encoder->finish(cmd_buffer_descriptor);

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

wgpu::raii::TextureView Application::GetNextSurfaceTextureView() {
    wgpu::SurfaceTexture surface_texture;
    m_surface->getCurrentTexture(&surface_texture);
    if (surface_texture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
        return {};
    }
    wgpu::Texture texture = surface_texture.texture;

    // Create a view for this surface texture
    wgpu::TextureViewDescriptor view_descriptor;
    view_descriptor.label               = "Surface texture view";
    view_descriptor.format              = texture.getFormat();
    view_descriptor.dimension           = wgpu::TextureViewDimension::_2D;
    view_descriptor.baseMipLevel        = 0;
    view_descriptor.mipLevelCount       = 1;
    view_descriptor.baseArrayLayer      = 0;
    view_descriptor.arrayLayerCount     = 1;
    view_descriptor.aspect              = wgpu::TextureAspect::All;
    wgpu::raii::TextureView target_view = texture.createView(view_descriptor);

#ifndef WEBGPU_BACKEND_WGPU
    // We no longer need the texture, only its view
    // (NB: with wgpu-native, surface textures must not be manually released)
    wgpuTextureRelease(surface_texture.texture);
#endif // WEBGPU_BACKEND_WGPU

    return target_view;
}