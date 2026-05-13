#ifndef BJTU_WGPU_RENDERER_WIREFRAMEPASS_H
#define BJTU_WGPU_RENDERER_WIREFRAMEPASS_H

#include "IRenderPass.h"
#include "webgpu-raii.hpp"

class RenderContext;

class WireframePass final : public IRenderPass {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderFrame& frame, const PassContext& context) override;

private:
    wgpu::raii::PipelineLayout m_wireframeLayout;
    wgpu::raii::PipelineLayout m_depthPrepassLayout;
    wgpu::raii::RenderPipeline m_wireframePipeline;
    wgpu::raii::RenderPipeline m_depthPrepassPipeline;
};

#endif // BJTU_WGPU_RENDERER_WIREFRAMEPASS_H
