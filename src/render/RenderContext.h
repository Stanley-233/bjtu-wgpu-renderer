#ifndef BJTU_WGPU_RENDERER_RENDERCONTEXT_H
#define BJTU_WGPU_RENDERER_RENDERCONTEXT_H

#include <memory>

#include <GLFW/glfw3.h>

#include "../webgpu-raii.hpp"

class RenderContext {
public:
    bool enableFrameDebug = false;

    RenderContext& SetWindowSize(int width, int height);

    RenderContext& SetSurfaceFormat(wgpu::TextureFormat format);

    bool Initialize();

    void Terminate();

    static void PollEvents();

    [[nodiscard]] bool IsRunning() const;

    [[nodiscard]] wgpu::raii::TextureView AcquireNextSurfaceView();

    [[nodiscard]] wgpu::raii::CommandEncoder BeginFrame() const;

    void SubmitAndPresent(wgpu::raii::CommandEncoder& encoder);

    [[nodiscard]] GLFWwindow* GetWindow() const;

    [[nodiscard]] wgpu::raii::Device& GetDevice();

    [[nodiscard]] wgpu::raii::Queue& GetQueue();

    [[nodiscard]] wgpu::TextureFormat GetSurfaceFormat() const;

    void GetDrawableSize(int& width, int& height) const;

private:
    static wgpu::RequiredLimits GetRequiredLimits(wgpu::Adapter adapter);
    void ConfigureSurface(uint32_t width, uint32_t height);
    void UpdateSurfaceConfigurationIfNeeded();

    int                                  m_windowWidth  = 640;
    int                                  m_windowHeight = 480;
    GLFWwindow*                          m_window       = nullptr;
    wgpu::raii::Device                   m_device;
    wgpu::raii::Queue                    m_queue;
    wgpu::raii::Surface                  m_surface;
    std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallbackHandle;
    wgpu::TextureFormat                  m_surfaceFormat = wgpu::TextureFormat::Undefined;
    uint32_t                             m_surfaceWidth  = 0;
    uint32_t                             m_surfaceHeight = 0;
};

#endif // BJTU_WGPU_RENDERER_RENDERCONTEXT_H
