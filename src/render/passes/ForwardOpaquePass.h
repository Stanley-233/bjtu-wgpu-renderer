#ifndef BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H
#define BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H

#include <vector>

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"
#include "webgpu-raii.hpp"

class RenderContext;

class ForwardOpaquePass final : public IRenderPass {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) override;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetSceneBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetObjectBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetMaterialBindGroupLayout() const;

private:
    struct SceneResources {
        wgpu::raii::Buffer    sceneUniformBuffer;
        wgpu::raii::BindGroup sceneBindGroup;
    };

    struct ObjectResources {
        wgpu::raii::Buffer    objectUniformBuffer;
        wgpu::raii::BindGroup objectBindGroup;
    };

    void EnsureSceneResources(RenderContext& ctx);

    void EnsureObjectResources(RenderContext& ctx, std::size_t objectCount);

    void UpdateSceneResources(RenderContext& ctx, const PassContext& context);

    void UpdateObjectResources(RenderContext& ctx, std::span<const PreparedDrawItem> drawItems);

    [[nodiscard]] wgpu::RenderPipeline SelectPipeline(EMaterialShadingModel shadingModel) const;

    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout m_objectBindGroupLayout;
    wgpu::raii::BindGroupLayout m_materialBindGroupLayout;
    wgpu::raii::Sampler         m_sceneAoSampler;
    wgpu::raii::RenderPipeline  m_unlitPipeline;
    wgpu::raii::RenderPipeline  m_lambertPipeline;
    SceneResources              m_sceneResources;
    std::vector<ObjectResources> m_objectResources;
};

#endif // BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H
