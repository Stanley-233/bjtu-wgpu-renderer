#ifndef BJTU_WGPU_RENDERER_TONEMAPPASS_H
#define BJTU_WGPU_RENDERER_TONEMAPPASS_H

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"

class RenderContext;

class ToneMapPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx);

    void SetSettings(const ToneMapSettings& settings);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_sceneColorBindGroupLayout;
    wgpu::raii::BindGroupLayout m_toneMapUniformBindGroupLayout;
    wgpu::raii::Sampler         m_sceneColorSampler;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_uniformBuffer;
    ToneMapSettings             m_settings{};
};

#endif // BJTU_WGPU_RENDERER_TONEMAPPASS_H
