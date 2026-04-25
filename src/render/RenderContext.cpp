#include "RenderContext.h"

#include <iostream>

#include <glfw3webgpu.h>
#include <magic_enum.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

using namespace wgpu;

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

    SurfaceConfiguration config = {};
    config.width                = m_windowWidth;
    config.height               = m_windowHeight;
    config.usage                = TextureUsage::RenderAttachment;
    if (m_surfaceFormat == TextureFormat::Undefined) {
        std::cout << "[Initialize] Surface format not specified, trying to get the preferred format..." << std::endl;
        m_surfaceFormat = m_surface->getPreferredFormat(*adapter);
    }
    config.format          = m_surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats     = nullptr;
    config.device          = *m_device;
    config.presentMode     = PresentMode::Fifo;
    config.alphaMode       = CompositeAlphaMode::Auto;

    std::cout << "Surface format: " << magic_enum::enum_name<WGPUTextureFormat>(m_surfaceFormat) << std::endl;
    m_surface->configure(config);

    SupportedLimits supportedLimits;
    adapter->getLimits(&supportedLimits);
    std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
    m_device->getLimits(&supportedLimits);
    std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    return true;
}

void RenderContext::Terminate() {
    ShutdownImGui();
    if (m_surface) {
        m_surface->unconfigure();
    }
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void RenderContext::PollEvents() {
    glfwPollEvents();
}

bool RenderContext::IsRunning() const {
    return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

raii::TextureView RenderContext::AcquireNextSurfaceView() {
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

bool RenderContext::InitializeImGui(GLFWwindow* window, raii::Device& device, const TextureFormat surfaceFormat) {
    if (m_imguiInitialized) {
        return true;
    }
    if (window == nullptr || !device) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOther(window, true)) {
        ImGui::DestroyContext();
        return false;
    }

    TextureFormat imguiSurfaceFormat = surfaceFormat;
    if (!ImGui_ImplWGPU_Init(*device, 3, static_cast<WGPUTextureFormat>(imguiSurfaceFormat))) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_imguiInitialized = true;
    return true;
}

void RenderContext::BeginImGuiFrame(const char* activeSceneName) {
    if (!m_imguiInitialized) {
        return;
    }

    m_imguiDrawDataReady = false;
    m_imguiActiveSceneName = activeSceneName == nullptr ? "Unknown" : activeSceneName;

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGui::Begin("Hello, world!");
    ImGui::Text("Active scene: %s", m_imguiActiveSceneName.c_str());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    if (ImGui::Button("Button")) {
        ++m_imguiButtonClickCount;
    }
    ImGui::SameLine();
    ImGui::Text("clicks = %d", m_imguiButtonClickCount);
    ImGui::Checkbox("Sample checkbox", &m_imguiCheckboxEnabled);
    ImGui::End();

    ImGui::Render();
    m_imguiDrawDataReady = true;
}

void RenderContext::RenderImGui(raii::RenderPassEncoder& renderPass) {
    if (!m_imguiInitialized || !m_imguiDrawDataReady || !renderPass) {
        return;
    }
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), *renderPass);
    m_imguiDrawDataReady = false;
}

void RenderContext::ShutdownImGui() {
    if (!m_imguiInitialized) {
        return;
    }
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_imguiInitialized = false;
    m_imguiDrawDataReady = false;
}

bool RenderContext::WantCaptureKeyboard() const {
    if (!m_imguiInitialized) {
        return false;
    }
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
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
