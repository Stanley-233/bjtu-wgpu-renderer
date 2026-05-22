#include "LegacyGuiRenderer.h"

#include <cmath>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

namespace {
constexpr float kPi = 3.14159265358979323846f;

float RadiansToDegrees(const float radians) {
    return radians * (180.0f / kPi);
}

float DegreesToRadians(const float degrees) {
    return degrees * (kPi / 180.0f);
}
}

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

void LegacyGuiRenderer::SetDirectionalLightCallbacks(DirectionalLightGetter getter, DirectionalLightSetter setter) {
    m_directionalLightGetter = getter;
    m_directionalLightSetter = setter;
}

void LegacyGuiRenderer::SetCameraInfoCallback(CameraGetter getter) {
    m_cameraGetter = getter;
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
        ImGui::Separator();
    }

    // ===== 摄像机信息（只读显示）=====
    if (ImGui::CollapsingHeader("Camera Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_cameraGetter) {
            CameraGuiData camera = m_cameraGetter();

            ImGui::Text("Position:");
            ImGui::PushID("cam_pos");
            ImGui::Text("  X: %.2f", camera.position.x);
            ImGui::SameLine();
            ImGui::Text("Y: %.2f", camera.position.y);
            ImGui::SameLine();
            ImGui::Text("Z: %.2f", camera.position.z);
            ImGui::PopID();

            ImGui::Text("Target:");
            ImGui::PushID("cam_target");
            ImGui::Text("  X: %.2f", camera.target.x);
            ImGui::SameLine();
            ImGui::Text("Y: %.2f", camera.target.y);
            ImGui::SameLine();
            ImGui::Text("Z: %.2f", camera.target.z);
            ImGui::PopID();
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No camera data available");
        }
    }

    ImGui::Separator();

    // ===== 平行光控制 =====
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_directionalLightGetter && m_directionalLightSetter) {
            DirectionalLightGuiData light = m_directionalLightGetter();
            const float dirLength = std::sqrt(
                light.direction.x * light.direction.x
                + light.direction.y * light.direction.y
                + light.direction.z * light.direction.z);
            const glm::vec3 normalizedDirection = dirLength > 0.001f
                                                      ? light.direction / dirLength
                                                      : glm::vec3{0.0f, -1.0f, 0.0f};
            float yawDegrees = RadiansToDegrees(std::atan2(normalizedDirection.x, -normalizedDirection.z));
            float pitchDegrees = RadiansToDegrees(std::asin(normalizedDirection.y));

            // 方向控制改为角度，避免直接拖方向向量分量难以理解。
            ImGui::Text("Direction Angles:");
            ImGui::PushID("light_dir");
            bool directionChanged = false;
            directionChanged |= ImGui::SliderFloat("Yaw", &yawDegrees, -180.0f, 180.0f, "%.1f deg");
            directionChanged |= ImGui::SliderFloat("Pitch", &pitchDegrees, -89.0f, 89.0f, "%.1f deg");
            ImGui::PopID();

            if (directionChanged) {
                const float yawRadians = DegreesToRadians(yawDegrees);
                const float pitchRadians = DegreesToRadians(pitchDegrees);
                const float cosPitch = std::cos(pitchRadians);
                light.direction.x = std::sin(yawRadians) * cosPitch;
                light.direction.y = std::sin(pitchRadians);
                light.direction.z = -std::cos(yawRadians) * cosPitch;
                m_directionalLightSetter(light);
            }

            ImGui::Text("Direction Vector:");
            ImGui::Text("  X: %.3f", normalizedDirection.x);
            ImGui::SameLine();
            ImGui::Text("Y: %.3f", normalizedDirection.y);
            ImGui::SameLine();
            ImGui::Text("Z: %.3f", normalizedDirection.z);
            ImGui::Text("  Length: %.3f", dirLength);
            ImGui::Text("  Yaw: %.1f deg  Pitch: %.1f deg", yawDegrees, pitchDegrees);

            ImGui::Spacing();

            // 强度控制
            ImGui::Text("Intensity:");
            ImGui::PushID("light_intensity");
            bool intensityChanged = ImGui::SliderFloat("##Intensity", &light.intensity, 0.0f, 5.0f, "%.2f");
            ImGui::PopID();
            if (intensityChanged) {
                m_directionalLightSetter(light);
            }

            ImGui::Spacing();

            // 颜色控制
            ImGui::Text("Color:");
            ImGui::PushID("light_color");
            bool colorChanged = false;
            colorChanged |= ImGui::SliderFloat("R", &light.color.r, 0.0f, 1.0f, "%.2f");
            colorChanged |= ImGui::SliderFloat("G", &light.color.g, 0.0f, 1.0f, "%.2f");
            colorChanged |= ImGui::SliderFloat("B", &light.color.b, 0.0f, 1.0f, "%.2f");
            ImGui::PopID();

            if (colorChanged) {
                m_directionalLightSetter(light);
            }

            ImGui::Spacing();

            // 颜色预览
            ImGui::ColorButton("Preview", ImVec4(light.color.r, light.color.g, light.color.b, 1.0f));
            ImGui::SameLine();
            ImGui::Text("Preview");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No directional light data available");
        }
    }

    ImGui::End();
}
