#ifndef BJTU_WGPU_RENDERER_TRANSLATIONCAMERACONTROLLER_H
#define BJTU_WGPU_RENDERER_TRANSLATIONCAMERACONTROLLER_H

#include "scene/camera/CameraController.h"

class TranslationCameraController final : public CameraController {
public:
    void OnMoveInput(const CameraMoveInputEvent& event) override;

    void Update(float dt, Camera& camera) override;

private:
    static constexpr float kMoveSpeed = 2.5f;

    float m_moveForward = 0.0f;
    float m_moveRight = 0.0f;
    float m_moveUp = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_TRANSLATIONCAMERACONTROLLER_H
