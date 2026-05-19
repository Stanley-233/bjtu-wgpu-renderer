#ifndef BJTU_WGPU_RENDERER_SHADOWPASS_H
#define BJTU_WGPU_RENDERER_SHADOWPASS_H

#include <vector>

#include "IRenderPass.h"
#include "webgpu-raii.hpp"

class RenderContext;

class ShadowPass final : public IRenderPass {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) override;

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

    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout m_objectBindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
    SceneResources               m_sceneResources;
    std::vector<ObjectResources> m_objectResources;
};

#endif // BJTU_WGPU_RENDERER_SHADOWPASS_H
