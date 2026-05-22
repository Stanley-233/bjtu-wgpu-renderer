#ifndef BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H
#define BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H

#include <GLFW/glfw3.h>
#include <functional>
#include <glm/vec3.hpp>

#include "webgpu-raii.hpp"

// 平行光数据结构（用于 GUI 控制）
struct DirectionalLightGuiData {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float     intensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
};

// 摄像机信息数据结构（用于 GUI 显示）
struct CameraGuiData {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 target{0.0f, 0.0f, -1.0f};
};

class LegacyGuiRenderer {
public:
    // 设置回调函数，用于获取/设置平行光数据
    using DirectionalLightGetter = std::function<DirectionalLightGuiData()>;
    using DirectionalLightSetter = std::function<void(const DirectionalLightGuiData&)>;
    using CameraGetter = std::function<CameraGuiData()>;

    bool Initialize(GLFWwindow *window, wgpu::raii::Device &device, WGPUTextureFormat surfaceFormat);

    void Shutdown();

    void BeginFrame(int drawableWidth, int drawableHeight);

    void EndFrame();

    void Render(wgpu::raii::RenderPassEncoder& renderPass);

    // 绘制调试面板（需要在 BeginFrame 后，EndFrame 前调用）
    void DrawDebugPanel();

    // 设置回调函数
    void SetDirectionalLightCallbacks(DirectionalLightGetter getter, DirectionalLightSetter setter);
    void SetCameraInfoCallback(CameraGetter getter);

    [[nodiscard]] bool WantCaptureKeyboard() const;

    [[nodiscard]] bool WantCaptureMouse() const;

private:
    void DrawDebugPanelContent();

    GLFWwindow* m_window = nullptr;
    bool        m_initialized = false;
    bool        m_drawDataReady = false;

    // 回调函数
    DirectionalLightGetter m_directionalLightGetter;
    DirectionalLightSetter m_directionalLightSetter;
    CameraGetter m_cameraGetter;
};

#endif // BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H
