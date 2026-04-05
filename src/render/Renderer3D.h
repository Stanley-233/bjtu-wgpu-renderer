#ifndef BJTU_WGPU_RENDERER_RENDERER3D_H
#define BJTU_WGPU_RENDERER_RENDERER3D_H

#include <cstdint>
#include <vector>

#include "../webgpu-raii.hpp"

class Camera;
class Object3D;
class RenderContext;

class Renderer3D {
public:
    void Initialize(RenderContext& ctx);

    void SyncScene(const std::vector<Object3D>& objects, const Camera& camera);

    void RenderFrame(RenderContext& ctx);

    void SetClearColor(double r, double g, double b, double a);

    void ResetGpuResources();

private:
    wgpu::raii::Buffer          m_uniformBuffer;
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::BindGroup       m_bindGroup;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_vertexBuffer;
    wgpu::raii::Buffer          m_indexBuffer;
    uint32_t                    m_indexCount = 0;
    wgpu::Color                 m_clearColor{0.03, 0.04, 0.06, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER3D_H
