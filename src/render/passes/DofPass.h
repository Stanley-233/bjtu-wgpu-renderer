#ifndef BJTU_WGPU_RENDERER_DOFPASS_H
#define BJTU_WGPU_RENDERER_DOFPASS_H

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"

class RenderContext;

class DofPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    void SetSettings(const DofSettings& settings);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

private:
    wgpu::raii::PipelineLayout  m_cocLayout;
    wgpu::raii::PipelineLayout  m_blurLayout;
    wgpu::raii::BindGroupLayout m_cocBindGroupLayout;
    wgpu::raii::BindGroupLayout m_blurBindGroupLayout;
    wgpu::raii::Sampler         m_colorSampler;
    wgpu::raii::Sampler         m_cocSampler;
    wgpu::raii::RenderPipeline  m_cocPipeline;
    wgpu::raii::RenderPipeline  m_blurPipeline;
    wgpu::raii::Buffer          m_uniformBuffer;
    DofSettings                 m_settings{};
};

#endif // BJTU_WGPU_RENDERER_DOFPASS_H
