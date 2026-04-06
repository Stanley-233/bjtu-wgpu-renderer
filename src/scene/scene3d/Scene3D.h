#ifndef BJTU_WGPU_RENDERER_SCENE3D_H
#define BJTU_WGPU_RENDERER_SCENE3D_H

#include <memory>
#include <vector>

#include "../../render/Renderer3D.h"
#include "../Scene.h"
#include "../camera/Camera.h"
#include "Object3D.h"

class Scene3D final : public IScene {
public:
    enum class ECameraMode {
        Perspective,
        Orthographic,
    };

    void Initialize(RenderContext& ctx) override;

    void Update(float dt) override;

    void Render(RenderContext& ctx) override;

    const char* Name() const override;

    void OnTransformAction(ETransformAction action, float amountX, float amountY) override;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) override;

    void ToggleCameraMode();

    [[nodiscard]] ECameraMode CameraMode() const;

private:
    void SetCameraMode(ECameraMode mode);

    static constexpr float kCameraMoveSpeed = 2.5f;

    ECameraMode             m_cameraMode = ECameraMode::Orthographic;
    std::unique_ptr<Camera> m_camera{};
    std::vector<Object3D>   m_objects{};
    Renderer3D              m_renderer{};
    float                   m_moveForward = 0.0f;
    float                   m_moveRight   = 0.0f;
    float                   m_moveUp      = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_SCENE3D_H
