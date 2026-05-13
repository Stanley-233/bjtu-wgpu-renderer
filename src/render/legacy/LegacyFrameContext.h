#ifndef BJTU_WGPU_RENDERER_LEGACYFRAMECONTEXT_H
#define BJTU_WGPU_RENDERER_LEGACYFRAMECONTEXT_H

#include "render/frame/SurfaceFrame.h"
#include "webgpu-raii.hpp"

class RenderContext;

class LegacyFrameContext {
public:
    explicit LegacyFrameContext(RenderContext& renderContext);

    [[nodiscard]] SurfaceFrame AcquireSurfaceFrame();

    [[nodiscard]] wgpu::raii::CommandEncoder CreateCommandEncoder() const;

    void SubmitAndPresent(SurfaceFrame& surfaceFrame, wgpu::raii::CommandEncoder& encoder);

    [[nodiscard]] wgpu::raii::Device& GetDevice() const;

    [[nodiscard]] wgpu::raii::Queue& GetQueue() const;

    void GetSurfaceSize(int& width, int& height) const;

private:
    RenderContext& m_renderContext;
};

#endif // BJTU_WGPU_RENDERER_LEGACYFRAMECONTEXT_H
