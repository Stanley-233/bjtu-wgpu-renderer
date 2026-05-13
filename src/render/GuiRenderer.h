#ifndef BJTU_WGPU_RENDERER_GUIRENDERER_H
#define BJTU_WGPU_RENDERER_GUIRENDERER_H

#include <GLFW/glfw3.h>

#include "webgpu-raii.hpp"

class GuiRenderer {
public:
    bool Initialize(GLFWwindow *window, wgpu::raii::Device &device, WGPUTextureFormat surfaceFormat);

    void Shutdown();

    void BeginFrame(int drawableWidth, int drawableHeight);

    void EndFrame();

    void Render(wgpu::raii::RenderPassEncoder& renderPass);

    [[nodiscard]] bool WantCaptureKeyboard() const;

private:
    GLFWwindow* m_window = nullptr;
    bool        m_initialized = false;
    bool        m_drawDataReady = false;
};

#endif // BJTU_WGPU_RENDERER_GUIRENDERER_H
