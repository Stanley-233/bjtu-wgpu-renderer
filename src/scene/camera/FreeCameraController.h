#ifndef BJTU_WGPU_RENDERER_FREECAMERACONTROLLER_H
#define BJTU_WGPU_RENDERER_FREECAMERACONTROLLER_H

#include "scene/camera/CameraController.h"

class FreeCameraController final : public CameraController {
public:
    void OnMoveInput(const CameraMoveInputEvent& event) override;

    void OnLookInput(const CameraLookInputEvent& event) override;

    void Update(float dt, Camera& camera) override;

private:
    static constexpr float kMoveSpeed = 2.5f;

    // TODO：后续在这里缓存 yaw/pitch、鼠标灵敏度，以及 RMB 视角控制模式状态。
    float m_moveForward = 0.0f;
    float m_moveRight = 0.0f;
    float m_moveUp = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_FREECAMERACONTROLLER_H
