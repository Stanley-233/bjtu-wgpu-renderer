#pragma once

#include <cstdint>
#include <vector>

#include "resource/legacy/LegacyMeshData3D.h"

using MeshVertex = Vertex3D;

struct MeshPrimitiveRange {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

struct MeshAsset {
    std::vector<MeshVertex>         vertices;
    std::vector<uint16_t>           indices;
    std::vector<MeshPrimitiveRange> primitiveRanges;
};
