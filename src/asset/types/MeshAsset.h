#pragma once

#include <cstdint>
#include <vector>

#include "AssetVertex3D.h"

struct MeshPrimitiveRange {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

struct MeshAsset {
    std::vector<AssetVertex3D>      vertices;
    std::vector<uint16_t>           indices;
    std::vector<MeshPrimitiveRange> primitiveRanges;
};
