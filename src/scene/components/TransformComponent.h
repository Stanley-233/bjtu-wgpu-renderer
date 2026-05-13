#ifndef BJTU_WGPU_RENDERER_TRANSFORMCOMPONENT_H
#define BJTU_WGPU_RENDERER_TRANSFORMCOMPONENT_H

#include "math/Transform3D.h"

struct TransformComponent {
    Transform3D transform = Transform3D::Identity();
};

#endif // BJTU_WGPU_RENDERER_TRANSFORMCOMPONENT_H
