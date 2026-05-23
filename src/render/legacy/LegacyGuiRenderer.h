#ifndef BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H
#define BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H

#include <GLFW/glfw3.h>
#include <functional>

#include "webgpu-raii.hpp"

class LegacyGuiRenderer {
public:
    using DebugPanelContentDrawer = std::function<void()>;

    bool Initialize(GLFWwindow *window, wgpu::raii::Device &device, WGPUTextureFormat surfaceFormat);

    void Shutdown();

    void BeginFrame(int drawableWidth, int drawableHeight);

    void EndFrame();

    void Render(wgpu::raii::RenderPassEncoder& renderPass);

    // 绘制调试面板（需要在 BeginFrame 后，EndFrame 前调用）
    void DrawDebugPanel();

    void SetDebugPanelContentCallback(DebugPanelContentDrawer drawer);

    [[nodiscard]] bool WantCaptureKeyboard() const;

    [[nodiscard]] bool WantCaptureMouse() const;

private:
    void DrawDebugPanelContent();

    GLFWwindow* m_window = nullptr;
    bool        m_initialized = false;
    bool        m_drawDataReady = false;

    DebugPanelContentDrawer m_debugPanelContentDrawer;
};

#endif // BJTU_WGPU_RENDERER_LEGACYGUIRENDERER_H
