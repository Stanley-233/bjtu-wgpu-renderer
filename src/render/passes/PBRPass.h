#ifndef BJTU_WGPU_RENDERER_PBRPASS_H
#define BJTU_WGPU_RENDERER_PBRPASS_H

#include <vector>

#include "IRenderPass.h"
#include "render/scene/RenderUniformData.h"
#include "webgpu-raii.hpp"

class RenderContext;

class PBRPass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

    [[nodiscard]] const wgpu::raii::BindGroupLayout& GetMaterialBindGroupLayout() const;

private:
    struct SceneResources {
        wgpu::raii::Buffer    sceneUniformBuffer;
        wgpu::raii::Buffer    directionalShadowUniformBuffer;
        wgpu::raii::Buffer    debugUniformBuffer;
        wgpu::raii::BindGroup sceneBindGroup;
        wgpu::raii::BindGroup debugBindGroup;
    };

    struct ObjectResources {
        wgpu::raii::Buffer    objectUniformBuffer;
        wgpu::raii::BindGroup objectBindGroup;
    };

    void EnsureSceneResources(RenderContext& renderCtx);

    void EnsureObjectResources(RenderContext& renderCtx, std::size_t objectCount);

    void UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx);

    void UpdateObjectResources(RenderContext& renderCtx, std::span<const PreparedDrawItem> drawItems);

    [[nodiscard]] bool HasPbrDrawItems(std::span<const PreparedDrawItem> drawItems) const;

    [[nodiscard]] wgpu::RenderPipeline SelectPipeline(bool doubleSided) const;

    wgpu::raii::PipelineLayout   m_layout;
    wgpu::raii::BindGroupLayout  m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_objectBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_materialBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_debugBindGroupLayout;
    wgpu::raii::Sampler          m_sceneAoSampler;
    wgpu::raii::RenderPipeline   m_pipelineSingleSided;
    wgpu::raii::RenderPipeline   m_pipelineDoubleSided;
    SceneResources               m_sceneResources;
    std::vector<ObjectResources> m_objectResources;
};

#endif // BJTU_WGPU_RENDERER_PBRPASS_H
