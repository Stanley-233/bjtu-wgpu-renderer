#ifndef BJTU_WGPU_RENDERER_LEGACYSCENE3D_H
#define BJTU_WGPU_RENDERER_LEGACYSCENE3D_H

#include <memory>
#include <vector>

#include "render/legacy/LegacyRenderer3D.h"
#include "scene/IScene.h"
#include "scene/camera/Camera.h"
#include "Object3D.h"

class LegacyScene3D final : public IScene, public ITransform3DInputSink, public ICameraMoveInputSink {
public:
    enum class ECameraMode {
        Perspective,
        Orthographic,
    };

    bool Initialize(RenderContext& renderCtx) override;

    void Update(float dt) override;

    void Render(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) override;

    const char* Name() const override;

    void OnObjectTransform3DEvent(const ObjectTransform3DEvent& event) override;
    void OnObjectTransform3DStateEvent(const ObjectTransform3DStateEvent& event) override;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) override;
    void OnToggleCameraModeRequest(const ToggleCameraModeRequest& event);

    void ToggleCameraMode();

    [[nodiscard]] ECameraMode CameraMode() const;

    void RegisterInputHandlers(InputEventBus& eventBus) override;

    void UnregisterInputHandlers(InputEventBus& eventBus) override;

private:
    void SetCameraMode(ECameraMode mode);
    void AppendBuiltinCube();

    static constexpr float kCameraMoveSpeed = 2.5f;

    ECameraMode                 m_cameraMode = ECameraMode::Orthographic;
    std::unique_ptr<Camera>     m_camera{};
    std::vector<Object3D>       m_objects{};
    std::vector<Transform3D>    m_initialObjectTransforms{};
    LegacyRenderer3D            m_renderer{};
    ObjectTransform3DStateEvent m_objectTransformState{};
    float                       m_moveForward = 0.0f;
    float                       m_moveRight   = 0.0f;
    float                       m_moveUp      = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_LEGACYSCENE3D_H
