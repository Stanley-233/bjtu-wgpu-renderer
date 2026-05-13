#include "LegacyFrameContext.h"

#include "render/RenderContext.h"

LegacyFrameContext::LegacyFrameContext(RenderContext& renderContext)
    : m_renderContext(renderContext) {
}

SurfaceFrame LegacyFrameContext::AcquireSurfaceFrame() {
    return m_renderContext.AcquireSurfaceFrame();
}

wgpu::raii::CommandEncoder LegacyFrameContext::CreateCommandEncoder() const {
    return m_renderContext.CreateCommandEncoder();
}

void LegacyFrameContext::SubmitAndPresent(SurfaceFrame& surfaceFrame, wgpu::raii::CommandEncoder& encoder) {
    m_renderContext.Submit(encoder);
    m_renderContext.Present(surfaceFrame);
}

wgpu::raii::Device& LegacyFrameContext::GetDevice() const {
    return m_renderContext.GetDevice();
}

wgpu::raii::Queue& LegacyFrameContext::GetQueue() const {
    return m_renderContext.GetQueue();
}

void LegacyFrameContext::GetSurfaceSize(int& width, int& height) const {
    m_renderContext.GetSurfaceSize(width, height);
}
