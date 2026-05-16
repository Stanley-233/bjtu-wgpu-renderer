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

    static constexpr float kLookSensitivity = 0.002f;

    float m_moveForward = 0.0f;
    float m_moveRight  = 0.0f;
    float m_moveUp     = 0.0f;
    float m_yaw        = 0.0f;
    float m_pitch      = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_FREECAMERACONTROLLER_H