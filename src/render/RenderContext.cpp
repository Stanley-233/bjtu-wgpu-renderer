#include "RenderContext.h"

#include <algorithm>
#include <iostream>

#include "asset/types/AssetVertex3D.h"
#include "app/WindowContext.h"
#include <glfw3webgpu.h>
#include <magic_enum.hpp>

using namespace wgpu;

RenderContext& RenderContext::SetSurfaceFormat(TextureFormat format) {
    m_surfaceFormat = format;
    return *this;
}

bool RenderContext::Initialize(WindowContext& windowContext) {
    if (windowContext.GetWindow() == nullptr) {
        return false;
    }
    m_windowContext = &windowContext;

    raii::Instance instance = Instance(wgpuCreateInstance(nullptr));

    std::cout << "Requesting adapter..." << std::endl;
    m_surface                         = Surface(glfwGetWGPUSurface(*instance, windowContext.GetWindow()));
    RequestAdapterOptions adapterOpts = {};
    adapterOpts.compatibleSurface     = *m_surface;
    raii::Adapter adapter             = instance->requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;
    DeviceDescriptor deviceDesc         = {};
    deviceDesc.label                    = "My Device";
    deviceDesc.requiredFeatureCount     = 0;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label       = "The default queue";
    deviceDesc.deviceLostCallback       = [](WGPUDeviceLostReason reason, char const* message, void*) {
        std::cout << "Device lost: reason " << reason;
        if (message != nullptr) {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };
    RequiredLimits requiredLimits = GetRequiredLimits(*adapter);
    deviceDesc.requiredLimits     = &requiredLimits;
    m_device                      = adapter->requestDevice(deviceDesc);
    std::cout << "Got device: " << m_device << std::endl;

    m_uncapturedErrorCallbackHandle = m_device->setUncapturedErrorCallback(
        [](ErrorType type, char const* message) {
            std::cout << "Uncaptured device error: type " << type;
            if (message != nullptr) {
                std::cout << " (" << message << ")";
            }
            std::cout << std::endl;
        });

    m_queue = m_device->getQueue();

    int framebufferWidth  = 0;
    int framebufferHeight = 0;
    GetSurfaceSize(framebufferWidth, framebufferHeight);

    if (m_surfaceFormat == TextureFormat::Undefined) {
        std::cout << "[Initialize] Surface format not specified, trying to get the preferred format..." << std::endl;
        m_surfaceFormat = m_surface->getPreferredFormat(*adapter);
    }
    std::cout << "Surface format: " << magic_enum::enum_name<WGPUTextureFormat>(m_surfaceFormat) << std::endl;
    ConfigureSurface(
        static_cast<uint32_t>(std::max(1, framebufferWidth)),
        static_cast<uint32_t>(std::max(1, framebufferHeight)));

    SupportedLimits supportedLimits;
    adapter->getLimits(&supportedLimits);
    std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
    m_device->getLimits(&supportedLimits);
    std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    return true;
}

void RenderContext::Shutdown() {
    if (m_surface) {
        m_surface->unconfigure();
    }
    m_surfaceWidth  = 0;
    m_surfaceHeight = 0;
    m_uncapturedErrorCallbackHandle.reset();
    m_surface = {};
    m_queue   = {};
    m_device  = {};
    m_windowContext = nullptr;
}

void RenderContext::ConfigureSurface(const uint32_t width, const uint32_t height) {
    if (!m_surface || !m_device || width == 0 || height == 0) {
        return;
    }

    SurfaceConfiguration config = {};
    config.width                = width;
    config.height               = height;
    config.usage                = TextureUsage::RenderAttachment;
    config.format               = m_surfaceFormat;
    config.viewFormatCount      = 0;
    config.viewFormats          = nullptr;
    config.device               = *m_device;
    config.presentMode          = PresentMode::Fifo;
    config.alphaMode            = CompositeAlphaMode::Auto;

    m_surface->configure(config);
    m_surfaceWidth  = width;
    m_surfaceHeight = height;
    std::cout << "Surface size: " << width << "x" << height << std::endl;
}

void RenderContext::UpdateSurfaceConfigurationIfNeeded() {
    int drawableWidth  = 0;
    int drawableHeight = 0;
    GetSurfaceSize(drawableWidth, drawableHeight);
    const uint32_t targetWidth  = static_cast<uint32_t>(std::max(1, drawableWidth));
    const uint32_t targetHeight = static_cast<uint32_t>(std::max(1, drawableHeight));
    if (targetWidth == m_surfaceWidth && targetHeight == m_surfaceHeight) {
        return;
    }
    ConfigureSurface(targetWidth, targetHeight);
}

SurfaceFrame RenderContext::AcquireSurfaceFrame() {
    UpdateSurfaceConfigurationIfNeeded();
    SurfaceFrame frame{};
    if (!m_surface) {
        return frame;
    }

    m_surface->getCurrentTexture(&frame.surfaceTexture);
    if (frame.surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
        return {};
    }
    Texture texture = frame.surfaceTexture.texture;

    TextureViewDescriptor viewDescriptor;
    viewDescriptor.label           = "Surface texture view";
    viewDescriptor.format          = texture.getFormat();
    viewDescriptor.dimension       = TextureViewDimension::_2D;
    viewDescriptor.baseMipLevel    = 0;
    viewDescriptor.mipLevelCount   = 1;
    viewDescriptor.baseArrayLayer  = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect          = TextureAspect::All;
    frame.view                     = texture.createView(viewDescriptor);
    frame.surfaceWidth             = static_cast<int>(m_surfaceWidth);
    frame.surfaceHeight            = static_cast<int>(m_surfaceHeight);
    return frame;
}

raii::CommandEncoder RenderContext::CreateCommandEncoder() const {
    CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label                    = "My command encoder";
    return CommandEncoder(wgpuDeviceCreateCommandEncoder(*m_device, &encoderDesc));
}

void RenderContext::Submit(raii::CommandEncoder& encoder) {
    CommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.label                   = "Command buffer";
    raii::CommandBuffer commandBuffer           = encoder->finish(cmdBufferDescriptor);

    if (enableFrameDebug) {
        std::cout << "Submitting command..." << std::endl;
    }
    m_queue->submit(1, commandBuffer.Ptr());
    if (enableFrameDebug) {
        std::cout << "Command submitted." << std::endl;
    }

#ifdef WEBGPU_BACKEND_DAWN
    m_device->tick();
#elif WEBGPU_BACKEND_WGPU
    m_device->poll(false);
#endif
}

void RenderContext::Present(SurfaceFrame& surfaceFrame) {
    if (surfaceFrame.surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success
        || surfaceFrame.surfaceTexture.texture == nullptr) {
        return;
    }

#ifndef __EMSCRIPTEN__
    m_surface->present();
#endif

#ifndef WEBGPU_BACKEND_WGPU
    wgpuTextureRelease(surfaceFrame.surfaceTexture.texture);
#endif

    surfaceFrame.surfaceTexture.texture = nullptr;
    surfaceFrame.view = {};
}

raii::Device& RenderContext::GetDevice() {
    return m_device;
}

raii::Queue& RenderContext::GetQueue() {
    return m_queue;
}

TextureFormat RenderContext::GetSurfaceFormat() const {
    return m_surfaceFormat;
}

void RenderContext::GetSurfaceSize(int& width, int& height) const {
    width  = 0;
    height = 0;
    if (m_windowContext != nullptr) {
        m_windowContext->GetDrawableSize(width, height);
    }
}

RequiredLimits RenderContext::GetRequiredLimits(Adapter adapter) {
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    RequiredLimits requiredLimits = Default;
    // Emscripten 不支持
    // requiredLimits.limits = supportedLimits.limits;
    requiredLimits.limits.maxVertexAttributes        = 4;
    requiredLimits.limits.maxVertexBuffers           = 1;
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(AssetVertex3D);
    requiredLimits.limits.maxBindGroups              = 2;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 3 * 16 * sizeof(float);

#ifndef __EMSCRIPTEN__
    requiredLimits.limits.maxInterStageShaderComponents = 6;
#endif

    return requiredLimits;
}
