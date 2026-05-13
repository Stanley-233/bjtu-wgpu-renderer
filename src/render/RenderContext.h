#ifndef BJTU_WGPU_RENDERER_RENDERCONTEXT_H
#define BJTU_WGPU_RENDERER_RENDERCONTEXT_H

#include <memory>

#include "frame/SurfaceFrame.h"
#include "webgpu-raii.hpp"

class WindowContext;

class RenderContext {
public:
    bool enableFrameDebug = false;

    RenderContext& SetSurfaceFormat(wgpu::TextureFormat format);

    bool Initialize(WindowContext& windowContext);

    void Shutdown();

    [[nodiscard]] SurfaceFrame AcquireSurfaceFrame();

    [[nodiscard]] wgpu::raii::CommandEncoder CreateCommandEncoder() const;

    void Submit(wgpu::raii::CommandEncoder& encoder);

    void Present(SurfaceFrame& surfaceFrame);

    [[nodiscard]] wgpu::raii::Device& GetDevice();

    [[nodiscard]] wgpu::raii::Queue& GetQueue();

    [[nodiscard]] wgpu::TextureFormat GetSurfaceFormat() const;

    void GetSurfaceSize(int& width, int& height) const;

private:
    static wgpu::RequiredLimits GetRequiredLimits(wgpu::Adapter adapter);
    void ConfigureSurface(uint32_t width, uint32_t height);
    void UpdateSurfaceConfigurationIfNeeded();

    WindowContext*                       m_windowContext = nullptr;
    wgpu::raii::Device                   m_device;
    wgpu::raii::Queue                    m_queue;
    wgpu::raii::Surface                  m_surface;
    std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallbackHandle;
    wgpu::TextureFormat                  m_surfaceFormat = wgpu::TextureFormat::Undefined;
    uint32_t                             m_surfaceWidth  = 0;
    uint32_t                             m_surfaceHeight = 0;
};

#endif // BJTU_WGPU_RENDERER_RENDERCONTEXT_H
