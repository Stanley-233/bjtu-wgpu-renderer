#ifndef BJTU_WGPU_RENDERER_SSAOPASS_H
#define BJTU_WGPU_RENDERER_SSAOPASS_H

#include "IRenderPass.h"

class SSAOPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx);

    void SetEnabled(bool enabled);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_ssaoBindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_uniformBuffer;
    bool                        m_enabled = true;
};

#endif // BJTU_WGPU_RENDERER_SSAOPASS_H
