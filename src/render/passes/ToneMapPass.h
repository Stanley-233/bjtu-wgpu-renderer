#ifndef BJTU_WGPU_RENDERER_TONEMAPPASS_H
#define BJTU_WGPU_RENDERER_TONEMAPPASS_H

#include "IRenderPass.h"

class RenderContext;

class ToneMapPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_sceneColorBindGroupLayout;
    wgpu::raii::Sampler         m_sceneColorSampler;
    wgpu::raii::RenderPipeline  m_pipeline;
};

#endif // BJTU_WGPU_RENDERER_TONEMAPPASS_H
