#ifndef BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
#define BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H

#include "asset/AssetId.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"

struct StaticMeshComponent {
    AssetId<MeshAsset>     mesh{};
    AssetId<MaterialAsset> material{};
};

#endif // BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
