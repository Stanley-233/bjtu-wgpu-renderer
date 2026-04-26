#include "GuiRenderer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

bool GuiRenderer::Initialize(GLFWwindow* window, wgpu::raii::Device& device, const wgpu::TextureFormat surfaceFormat) {
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

    if (!ImGui_ImplWGPU_Init(*device, 3, static_cast<WGPUTextureFormat>(surfaceFormat))) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_window = window;
    m_initialized = true;
    return true;
}

void GuiRenderer::Shutdown() {
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

void GuiRenderer::BeginFrame(const int drawableWidth, const int drawableHeight) {
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

void GuiRenderer::EndFrame() {
    if (!m_initialized) {
        return;
    }
    ImGui::Render();
    m_drawDataReady = true;
}

void GuiRenderer::Render(wgpu::raii::RenderPassEncoder& renderPass) {
    if (!m_initialized || !m_drawDataReady || !renderPass) {
        return;
    }
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), *renderPass);
    m_drawDataReady = false;
}

bool GuiRenderer::WantCaptureKeyboard() const {
    if (!m_initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}
