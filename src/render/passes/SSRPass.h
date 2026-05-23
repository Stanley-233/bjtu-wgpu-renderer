#ifndef BJTU_WGPU_RENDERER_SSRPASS_H
#define BJTU_WGPU_RENDERER_SSRPASS_H

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"

class RenderContext;

class SSRPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    void SetSettings(const SsrSettings& settings);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::Sampler         m_sceneSampler;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_uniformBuffer;
    SsrSettings                 m_settings{};
};

#endif // BJTU_WGPU_RENDERER_SSRPASS_H
