#ifndef BJTU_WGPU_RENDERER_FORWARDPASS_H
#define BJTU_WGPU_RENDERER_FORWARDPASS_H

#include "IRenderPass.h"
#include "webgpu-raii.hpp"

class RenderContext;

class ForwardPass final : public IRenderPass {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderFrame& frame, const PassContext& context) override;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetSceneBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetObjectBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetMaterialBindGroupLayout() const;

private:
    [[nodiscard]] wgpu::RenderPipeline SelectPipeline(EMaterialShadingModel shadingModel) const;

    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout m_objectBindGroupLayout;
    wgpu::raii::BindGroupLayout m_materialBindGroupLayout;
    wgpu::raii::RenderPipeline  m_unlitPipeline;
    wgpu::raii::RenderPipeline  m_lambertPipeline;
};

#endif // BJTU_WGPU_RENDERER_FORWARDPASS_H
