#ifndef BJTU_WGPU_RENDERER_RENDERER_H
#define BJTU_WGPU_RENDERER_RENDERER_H

#include <vector>

#include <glm/mat4x4.hpp>

#include "render/scene/RenderScene.h"
#include "webgpu-raii.hpp"

class GuiRenderer;
class RenderContext;

class Renderer {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderContext& ctx, const RenderScene& scene, GuiRenderer& guiRenderer);

    void SetClearColor(double r, double g, double b, double a);

private:
    struct SceneUniform {
        glm::mat4 model{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
    };

    struct DrawItem {
        wgpu::raii::Buffer    vertexBuffer;
        wgpu::raii::Buffer    indexBuffer;
        wgpu::raii::Buffer    wireframeDepthIndexBuffer;
        wgpu::raii::Buffer    uniformBuffer;
        wgpu::raii::BindGroup bindGroup;
        uint64_t              vertexBufferSize = 0;
        uint64_t              indexBufferSize  = 0;
        uint64_t              wireframeDepthIndexBufferSize = 0;
        uint32_t              indexCount       = 0;
        uint32_t              wireframeDepthIndexCount = 0;
        uint32_t              sourceVertexCount = 0;
        uint32_t              sourceIndexCount  = 0;
        Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
        glm::mat4             model{1.0f};
    };

    void EnsureDepthResources(RenderContext& ctx, int width, int height);

    static DrawItem UploadMeshToGpu(RenderContext& ctx, const RenderObject& object);

    wgpu::raii::PipelineLayout  m_solidLayout;
    wgpu::raii::PipelineLayout  m_wireframeLayout;
    wgpu::raii::PipelineLayout  m_wireframeDepthPrepassLayout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::RenderPipeline  m_solidPipeline;
    wgpu::raii::RenderPipeline  m_wireframePipeline;
    wgpu::raii::RenderPipeline  m_wireframeDepthPrepassPipeline;
    wgpu::raii::Texture         m_depthTexture;
    wgpu::raii::TextureView     m_depthView;
    int                         m_depthWidth  = 0;
    int                         m_depthHeight = 0;
    std::vector<DrawItem>       m_drawItems;
    wgpu::Color                 m_clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER_H
