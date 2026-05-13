#ifndef BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
#define BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H

#include "resource/legacy/LegacyMeshData3D.h"
#include "scene/legacy/Object3D.h"

struct StaticMeshComponent {
    LegacyMeshData3D      mesh{};
    Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
};

#endif // BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
