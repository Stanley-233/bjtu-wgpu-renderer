#ifndef BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H
#define BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H

#include <vector>

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"
#include "webgpu-raii.hpp"

class RenderContext;

class ForwardOpaquePass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetSceneBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetObjectBindGroupLayout() const;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetMaterialBindGroupLayout() const;

private:
    struct SceneResources {
        wgpu::raii::Buffer    sceneUniformBuffer;
        wgpu::raii::Buffer    directionalShadowUniformBuffer;
        wgpu::raii::BindGroup sceneBindGroup;
    };

    struct ObjectResources {
        wgpu::raii::Buffer    objectUniformBuffer;
        wgpu::raii::BindGroup objectBindGroup;
    };

    void EnsureSceneResources(RenderContext& renderCtx);

    void EnsureObjectResources(RenderContext& renderCtx, std::size_t objectCount);

    void UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx);

    void UpdateObjectResources(RenderContext& renderCtx, std::span<const PreparedDrawItem> drawItems);

    [[nodiscard]] wgpu::RenderPipeline SelectPipeline(EMaterialShadingModel shadingModel, bool doubleSided) const;

    wgpu::raii::PipelineLayout   m_layout;
    wgpu::raii::BindGroupLayout  m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_objectBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_materialBindGroupLayout;
    wgpu::raii::Sampler          m_sceneAoSampler;
    wgpu::raii::RenderPipeline   m_unlitPipelineSingleSided;
    wgpu::raii::RenderPipeline   m_unlitPipelineDoubleSided;
    wgpu::raii::RenderPipeline   m_lambertPipelineSingleSided;
    wgpu::raii::RenderPipeline   m_lambertPipelineDoubleSided;
    SceneResources               m_sceneResources;
    std::vector<ObjectResources> m_objectResources;
};

#endif // BJTU_WGPU_RENDERER_FORWARDOPAQUEPASS_H
