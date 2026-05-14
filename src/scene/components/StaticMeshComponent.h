#ifndef BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
#define BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H

#include "asset/AssetHandle.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"
#include "scene/legacy/Object3D.h"

struct StaticMeshComponent {
    AssetHandle<MeshAsset>     mesh{};
    AssetHandle<MaterialAsset> material{};
    Object3D::ERenderMode      renderMode = Object3D::ERenderMode::Solid;
};

#endif // BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
