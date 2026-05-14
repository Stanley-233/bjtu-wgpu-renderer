#pragma once

#include <vector>

#include <glm/mat4x4.hpp>

#include "asset/AssetId.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"

struct ModelPrimitiveAsset {
    AssetId<MeshAsset>     mesh{};
    AssetId<MaterialAsset> material{};
    uint32_t               primitiveIndex = 0;
};

struct ModelNodeAsset {
    glm::mat4 modelMatrix{1.0f};
    std::vector<ModelPrimitiveAsset> primitives;
};

struct ModelAsset {
    std::vector<ModelNodeAsset> nodes;
};
