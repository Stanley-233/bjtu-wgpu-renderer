#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

#include "asset/AssetId.h"
#include "ImageAsset.h"

enum class EMaterialShadingModel : uint32_t {
    Unlit = 0,
    Lambert = 1,
};

struct MaterialAsset {
    EMaterialShadingModel shadingModel{EMaterialShadingModel::Unlit};
    glm::vec4           baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    AssetId<ImageAsset> baseColorTexture{};
    bool                useVertexColor = true;
};
