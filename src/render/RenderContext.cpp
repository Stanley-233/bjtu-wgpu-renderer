#include "RenderContext.h"

#include <algorithm>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <cmath>
#include <emscripten/html5.h>
#endif

#include <glfw3webgpu.h>
#include <magic_enum.hpp>

using namespace wgpu;

#ifdef __EMSCRIPTEN__
namespace {
void ConfigureCanvasForHighDpi(const int windowWidth, const int windowHeight) {
    // Keep CSS size in logical pixels while increasing backing store resolution.
    emscripten_set_element_css_size("#canvas", static_cast<double>(windowWidth), static_cast<double>(windowHeight));

    double cssWidth  = static_cast<double>(windowWidth);
    double cssHeight = static_cast<double>(windowHeight);
    if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || cssWidth <= 0.0 || cssHeight <= 0.0) {
        cssWidth  = static_cast<double>(windowWidth);
        cssHeight = static_cast<double>(windowHeight);
    }

    const double dpr = std::max(1.0, emscripten_get_device_pixel_ratio());
    const int pixelWidth  = std::max(1, static_cast<int>(std::lround(cssWidth * dpr)));
    const int pixelHeight = std::max(1, static_cast<int>(std::lround(cssHeight * dpr)));
    emscripten_set_canvas_element_size("#canvas", pixelWidth, pixelHeight);
}

void UpdateCanvasBackingStoreForCurrentDpr() {
    double cssWidth  = 0.0;
    double cssHeight = 0.0;
    if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || cssWidth <= 0.0 || cssHeight <= 0.0) {
        return;
    }

    const double dpr = std::max(1.0, emscripten_get_device_pixel_ratio());
    const int desiredWidth  = std::max(1, static_cast<int>(std::lround(cssWidth * dpr)));
    const int desiredHeight = std::max(1, static_cast<int>(std::lround(cssHeight * dpr)));

    int currentWidth  = 0;
    int currentHeight = 0;
    if (emscripten_get_canvas_element_size("#canvas", &currentWidth, &currentHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || currentWidth != desiredWidth || currentHeight != desiredHeight) {
        emscripten_set_canvas_element_size("#canvas", desiredWidth, desiredHeight);
    }
}
} // namespace
#endif

RenderContext& RenderContext::SetWindowSize(int width, int height) {
    m_windowWidth  = width;
    m_windowHeight = height;
    return *this;
}

RenderContext& RenderContext::SetSurfaceFormat(TextureFormat format) {
    m_surfaceFormat = format;
    return *this;
}

bool RenderContext::Initialize() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "WebGPU Renderer", nullptr, nullptr);
    if (m_window == nullptr) {
        return false;
    }

#ifdef __EMSCRIPTEN__
    ConfigureCanvasForHighDpi(m_windowWidth, m_windowHeight);
#endif

    raii::Instance instance = Instance(wgpuCreateInstance(nullptr));

    std::cout << "Requesting adapter..." << std::endl;
    m_surface                         = Surface(glfwGetWGPUSurface(*instance, m_window));
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
    GetDrawableSize(framebufferWidth, framebufferHeight);

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

void RenderContext::Terminate() {
    if (m_surface) {
        m_surface->unconfigure();
    }
    m_surfaceWidth  = 0;
    m_surfaceHeight = 0;
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
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
#ifdef __EMSCRIPTEN__
    UpdateCanvasBackingStoreForCurrentDpr();
#endif
    int drawableWidth  = 0;
    int drawableHeight = 0;
    GetDrawableSize(drawableWidth, drawableHeight);
    const uint32_t targetWidth  = static_cast<uint32_t>(std::max(1, drawableWidth));
    const uint32_t targetHeight = static_cast<uint32_t>(std::max(1, drawableHeight));
    if (targetWidth == m_surfaceWidth && targetHeight == m_surfaceHeight) {
        return;
    }
    ConfigureSurface(targetWidth, targetHeight);
}

void RenderContext::PollEvents() {
    glfwPollEvents();
}

bool RenderContext::IsRunning() const {
    return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

raii::TextureView RenderContext::AcquireNextSurfaceView() {
    UpdateSurfaceConfigurationIfNeeded();
    SurfaceTexture surfaceTexture;
    m_surface->getCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
        return {};
    }
    Texture texture = surfaceTexture.texture;

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
    wgpuTextureRelease(surfaceTexture.texture);
#endif

    return targetView;
}

raii::CommandEncoder RenderContext::BeginFrame() const {
    CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label                    = "My command encoder";
    return CommandEncoder(wgpuDeviceCreateCommandEncoder(*m_device, &encoderDesc));
}

void RenderContext::SubmitAndPresent(raii::CommandEncoder& encoder) {
    CommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.label                   = "Command buffer";
    raii::CommandBuffer commandBuffer            = encoder->finish(cmdBufferDescriptor);

    if (enableFrameDebug) {
        std::cout << "Submitting command..." << std::endl;
    }
    m_queue->submit(1, commandBuffer.Ptr());
    if (enableFrameDebug) {
        std::cout << "Command submitted." << std::endl;
    }

#ifndef __EMSCRIPTEN__
    // 在浏览器里，当前帧的显示时机由浏览器自己的渲染循环控制
    // 代码只需要拿当前纹理、画进去、提交命令，浏览器会在合适的时候把结果展示出来
    // 见 main.cpp 设置 callback
    m_surface->present();
#endif

#ifdef WEBGPU_BACKEND_DAWN
    m_device->tick();
#elif WEBGPU_BACKEND_WGPU
    m_device->poll(false);
#endif
}

GLFWwindow* RenderContext::GetWindow() const {
    return m_window;
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

void RenderContext::GetDrawableSize(int& width, int& height) const {
    width  = 0;
    height = 0;
#ifdef __EMSCRIPTEN__
    if (emscripten_get_canvas_element_size("#canvas", &width, &height) == EMSCRIPTEN_RESULT_SUCCESS
        && width > 0 && height > 0) {
        return;
    }
#endif

    glfwGetFramebufferSize(m_window, &width, &height);
    if (width <= 0) {
        width = m_windowWidth;
    }
    if (height <= 0) {
        height = m_windowHeight;
    }
}

RequiredLimits RenderContext::GetRequiredLimits(Adapter adapter) {
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    RequiredLimits requiredLimits = Default;
    // Emscripten 不支持
    // requiredLimits.limits = supportedLimits.limits;
    requiredLimits.limits.maxVertexAttributes        = 3;
    requiredLimits.limits.maxVertexBuffers           = 1;
    requiredLimits.limits.maxVertexBufferArrayStride = 6 * sizeof(float);
    requiredLimits.limits.maxBindGroups              = 2;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 3 * 16 * sizeof(float);

#ifndef __EMSCRIPTEN__
    requiredLimits.limits.maxInterStageShaderComponents = 6;
#endif

    return requiredLimits;
}
