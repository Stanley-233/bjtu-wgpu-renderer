#ifndef BJTU_WGPU_RENDERER_SCENE2D_H
#define BJTU_WGPU_RENDERER_SCENE2D_H

#include <cstddef>

#include "scene/IScene.h"
#include "math/Transform2D.h"
#include "webgpu-raii.hpp"

class Scene2D : public IScene, public ITransform2DInputSink {
public:
    bool Initialize(RenderContext& renderCtx) override;

    void Update(float dt) override;

    void Render(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) override;

    const char* Name() const override;

    void RegisterInputHandlers(InputEventBus& eventBus) override;

    void UnregisterInputHandlers(InputEventBus& eventBus) override;

    void OnTransformInputEvent(const TransformActionEvent& event) override;
    void OnTransform2DStateEvent(const Transform2DStateEvent& event) override;

private:
    struct SceneUniformData {
        Transform2D::Mat3Uniform transform{};
        float                    aspect = 1.0f;
        float                    padding[3]{};
    };

    static_assert(
        offsetof(SceneUniformData, aspect) == Transform2D::kMat3UniformSize,
        "Scene2D aspect uniform offset must match WGSL layout");
    static_assert(sizeof(SceneUniformData) == 16 * sizeof(float), "Scene2D uniform size must be 64 bytes");

    bool InitializeBuffers(RenderContext& renderCtx);

    void InitializeBindGroups(RenderContext& renderCtx);

    void ApplyTransform(const Transform2D& t);

    void ResetTransform();

    void UploadTransformMatrix(const glm::mat3& matrix);

    void UpdateAspectUniform();

    void UploadUniformData() const;

    RenderContext*              m_renderContext = nullptr;
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
    SceneUniformData            m_uniformData{};
    float                       m_lastAspect = 1.0f;
};

#endif // BJTU_WGPU_RENDERER_SCENE2D_H
