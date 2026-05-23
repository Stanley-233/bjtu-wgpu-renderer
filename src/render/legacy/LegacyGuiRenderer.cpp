#include "LegacyGuiRenderer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

bool LegacyGuiRenderer::Initialize(GLFWwindow *window, wgpu::raii::Device &device, const WGPUTextureFormat surfaceFormat) {
    if (m_initialized) {
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

    if (!ImGui_ImplWGPU_Init(*device, 3, surfaceFormat)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_window = window;
    m_initialized = true;
    return true;
}

void LegacyGuiRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
    m_drawDataReady = false;
    m_window = nullptr;
}

void LegacyGuiRenderer::BeginFrame(const int drawableWidth, const int drawableHeight) {
    if (!m_initialized) {
        return;
    }
    m_drawDataReady = false;

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    const float scaleX = (windowWidth > 0) ? static_cast<float>(drawableWidth) / static_cast<float>(windowWidth) : 1.0f;
    const float scaleY = (windowHeight > 0) ? static_cast<float>(drawableHeight) / static_cast<float>(windowHeight) : 1.0f;
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(scaleX, scaleY);
}

void LegacyGuiRenderer::EndFrame() {
    if (!m_initialized) {
        return;
    }
    ImGui::Render();
    m_drawDataReady = true;
}

void LegacyGuiRenderer::Render(wgpu::raii::RenderPassEncoder& renderPass) {
    if (!m_initialized || !m_drawDataReady || !renderPass) {
        return;
    }
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), *renderPass);
    m_drawDataReady = false;
}

bool LegacyGuiRenderer::WantCaptureKeyboard() const {
    if (!m_initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool LegacyGuiRenderer::WantCaptureMouse() const {
    if (!m_initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

void LegacyGuiRenderer::SetDebugPanelContentCallback(DebugPanelContentDrawer drawer) {
    m_debugPanelContentDrawer = std::move(drawer);
}

void LegacyGuiRenderer::DrawDebugPanel() {
    if (!m_initialized) {
        return;
    }
    DrawDebugPanelContent();
}

void LegacyGuiRenderer::DrawDebugPanelContent() {
    // 移除固定标志，允许窗口自由拖动和调整大小
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

    // 设置初始位置在右上角，给合并后的面板预留更大的初始空间
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 370.0f, 10.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 520.0f), ImGuiCond_Once);

    if (!ImGui::Begin("Debug Panel", nullptr, windowFlags)) {
        ImGui::End();
        return;
    }

    if (m_debugPanelContentDrawer) {
        m_debugPanelContentDrawer();
    }

    ImGui::End();
}
