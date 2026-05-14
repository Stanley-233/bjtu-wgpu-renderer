#ifndef BJTU_WGPU_RENDERER_CAMERACONTROLLER_H
#define BJTU_WGPU_RENDERER_CAMERACONTROLLER_H

#include "scene/IScene.h"

class Camera;

class CameraController {
public:
    virtual ~CameraController() = default;

    virtual void OnMoveInput(const CameraMoveInputEvent& event) = 0;

    virtual void Update(float dt, Camera& camera) = 0;
};

#endif // BJTU_WGPU_RENDERER_CAMERACONTROLLER_H
