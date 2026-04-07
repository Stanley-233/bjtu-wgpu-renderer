#ifndef BJTU_WGPU_RENDERER_RENDERER3D_H
#define BJTU_WGPU_RENDERER_RENDERER3D_H

#include <vector>

#include <glm/mat4x4.hpp>

#include "../webgpu-raii.hpp"

class Camera;
class Object3D;
class RenderContext;

class Renderer3D {
public:
    void Initialize(RenderContext& ctx);

    void SyncScene(RenderContext& ctx, const std::vector<Object3D>& objects, const Camera& camera);

    void RenderFrame(RenderContext& ctx);

    void SetClearColor(double r, double g, double b, double a);

    void ResetGpuResources();

private:
    struct SceneUniform {
        glm::mat4 model{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
    };

    struct DrawItem {
        wgpu::raii::Buffer vertexBuffer;
        wgpu::raii::Buffer indexBuffer;
        uint64_t           vertexBufferSize = 0;
        uint64_t           indexBufferSize  = 0;
        uint32_t           indexCount       = 0;
        glm::mat4          model{1.0f};
    };

    void EnsureDepthResources(RenderContext& ctx, int width, int height);

    static DrawItem UploadMeshToGpu(RenderContext& ctx, const Object3D& object);

    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Texture         m_depthTexture;
    wgpu::raii::TextureView     m_depthView;
    int                         m_depthWidth  = 0;
    int                         m_depthHeight = 0;
    glm::mat4                   m_view{1.0f};
    glm::mat4                   m_projection{1.0f};
    std::vector<DrawItem>       m_drawItems;
    wgpu::Color                 m_clearColor{0.03, 0.04, 0.06, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER3D_H
