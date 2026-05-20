#ifndef BJTU_WGPU_RENDERER_SSAOPASS_H
#define BJTU_WGPU_RENDERER_SSAOPASS_H

#include "IRenderPass.h"

class SSAOPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_ssaoBindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
};

#endif // BJTU_WGPU_RENDERER_SSAOPASS_H
