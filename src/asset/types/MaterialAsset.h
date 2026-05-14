#pragma once

#include <glm/vec4.hpp>

#include "asset/AssetId.h"
#include "ImageAsset.h"

struct MaterialAsset {
    glm::vec4           baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    AssetId<ImageAsset> baseColorTexture{};
};
