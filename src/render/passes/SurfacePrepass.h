#ifndef BJTU_WGPU_RENDERER_SURFACEPREPASS_H
#define BJTU_WGPU_RENDERER_SURFACEPREPASS_H

#include <vector>

#include "IRenderPass.h"
#include "webgpu-raii.hpp"

class RenderContext;

class SurfacePrepass final : public IRenderPass {
public:
    void Initialize(RenderContext& renderCtx);

    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;

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

    void EnsureSceneResources(RenderContext& renderCtx);

    void EnsureObjectResources(RenderContext& renderCtx, std::size_t objectCount);

    void UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx);

    void UpdateObjectResources(RenderContext& renderCtx, std::span<const PreparedDrawItem> drawItems);

    wgpu::raii::PipelineLayout   m_layout;
    wgpu::raii::BindGroupLayout  m_sceneBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_objectBindGroupLayout;
    wgpu::raii::BindGroupLayout  m_materialBindGroupLayout;
    wgpu::raii::RenderPipeline   m_pipeline;
    SceneResources               m_sceneResources;
    std::vector<ObjectResources> m_objectResources;
};

#endif // BJTU_WGPU_RENDERER_SURFACEPREPASS_H
