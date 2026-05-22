#ifndef BJTU_WGPU_RENDERER_SKYBOXPASS_H
#define BJTU_WGPU_RENDERER_SKYBOXPASS_H

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"

class RenderContext;

class SkyboxPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_uniformBuffer;
};

#endif // BJTU_WGPU_RENDERER_SKYBOXPASS_H
