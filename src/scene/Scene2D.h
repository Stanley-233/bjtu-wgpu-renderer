#ifndef BJTU_WGPU_RENDERER_SCENE2D_H
#define BJTU_WGPU_RENDERER_SCENE2D_H

#include "Scene.h"
#include "../math/Transform2D.h"
#include "../webgpu-raii.hpp"

class Scene2D : public IScene, public ITransform2DInputSink {
public:
    void Initialize(RenderContext& ctx) override;

    void Update(float dt) override;

    void Render(RenderContext& ctx) override;

    const char* Name() const override;

    void OnTransformInputEvent(const TransformActionEvent& event) override;
    void OnTransform2DStateEvent(const Transform2DStateEvent& event) override;

private:
    void InitializeBuffers(RenderContext& ctx);

    void InitializeBindGroups(RenderContext& ctx);

    void ApplyTransform(const Transform2D& t);

    void ResetTransform();

    void UploadTransformMatrix(const glm::mat3& matrix);

    RenderContext*              m_context = nullptr;
    wgpu::raii::Buffer          m_uniformBuffer;
    wgpu::raii::PipelineLayout  m_layout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::BindGroup       m_bindGroup;
    wgpu::raii::RenderPipeline  m_pipeline;
    wgpu::raii::Buffer          m_pointBuffer;
    wgpu::raii::Buffer          m_indexBuffer;
    uint32_t                    m_indexCount = 0;
    Transform2D                 m_transform;
    Transform2D                 m_pendingDelta;
    Transform2DStateEvent       m_transformState{};
};

#endif // BJTU_WGPU_RENDERER_SCENE2D_H
