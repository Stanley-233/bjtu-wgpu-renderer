#ifndef BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
#define BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H

#include "asset/AssetHandle.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"

struct StaticMeshComponent {
    AssetHandle<MeshAsset>     mesh{};
    AssetHandle<MaterialAsset> material{};
};

#endif // BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
