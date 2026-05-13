#ifndef BJTU_WGPU_RENDERER_FORWARDPASS_H
#define BJTU_WGPU_RENDERER_FORWARDPASS_H

#include "IRenderPass.h"
#include "webgpu-raii.hpp"

class RenderContext;

class ForwardPass final : public IRenderPass {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderFrame& frame, const PassContext& context) override;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetBindGroupLayout() const;

private:
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
};

#endif // BJTU_WGPU_RENDERER_FORWARDPASS_H
