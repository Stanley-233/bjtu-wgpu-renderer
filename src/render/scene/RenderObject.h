#ifndef BJTU_WGPU_RENDERER_RENDEROBJECT_H
#define BJTU_WGPU_RENDERER_RENDEROBJECT_H

#include <glm/mat4x4.hpp>

#include "asset/AssetId.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"
#include "resource/legacy/LegacyMeshData3D.h"

struct RenderObject {
    glm::mat4               worldMatrix{1.0f};
    AssetId<MeshAsset>      meshId{};
    AssetId<MaterialAsset>  materialId{};
    const MeshAsset*        mesh = nullptr;
    const MaterialAsset*    material = nullptr;
    const LegacyMeshData3D* legacyMesh = nullptr;
};

#endif // BJTU_WGPU_RENDERER_RENDEROBJECT_H
