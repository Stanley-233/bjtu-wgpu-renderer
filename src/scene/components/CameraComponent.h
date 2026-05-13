#ifndef BJTU_WGPU_RENDERER_CAMERACOMPONENT_H
#define BJTU_WGPU_RENDERER_CAMERACOMPONENT_H

#include <memory>

#include "scene/camera/Camera.h"

struct CameraComponent {
    std::unique_ptr<Camera> camera{};
    bool                    isPrimary = false;
};

#endif // BJTU_WGPU_RENDERER_CAMERACOMPONENT_H
