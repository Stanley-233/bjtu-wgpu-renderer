#include "ObjLoader.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#ifndef TINYOBJLOADER_IMPLEMENTATION
    #define TINYOBJLOADER_IMPLEMENTATION
#endif
#include <tiny_obj_loader.h>

namespace {
glm::vec3 HsvToRgb(const float h, const float s, const float v) {
    const float hh = h * 6.0f;
    const int   i  = static_cast<int>(std::floor(hh));
    const float f  = hh - static_cast<float>(i);
    const float p  = v * (1.0f - s);
    const float q  = v * (1.0f - s * f);
    const float t  = v * (1.0f - s * (1.0f - f));

    switch (i % 6) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        case 5: return {v, p, q};
        default: return {v, v, v};
    }
}

glm::vec3 FaceColorFromIndex(const uint32_t faceIndex) {
    // A deterministic integer hash to generate stable hue per face.
    uint32_t x = faceIndex;
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;

    constexpr float kInv24Bit = 1.0f / 16777216.0f;
    const float     hue        = static_cast<float>(x & 0x00FFFFFFu) * kInv24Bit;
    return HsvToRgb(hue, 0.78f, 0.95f);
}
} // namespace

bool ObjLoader::Load(const std::filesystem::path& path, MeshData3D& outMesh) {
    outMesh.vertices.clear();
    outMesh.indices.clear();

    tinyobj::attrib_t                attrib{};
    std::vector<tinyobj::shape_t>    shapes{};
    std::vector<tinyobj::material_t> materials{};
    std::string                      err{};

    std::string mtlSearchPath;
    if (const std::filesystem::path parent = path.parent_path(); !parent.empty()) {
        mtlSearchPath = parent.string();
        if (!mtlSearchPath.empty()) {
            const char last = mtlSearchPath.back();
            if (last != '/' && last != '\\') {
                mtlSearchPath.push_back(std::filesystem::path::preferred_separator);
            }
        }
    }

    const bool parsed = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &err,
        path.string().c_str(),
        mtlSearchPath.empty() ? nullptr : mtlSearchPath.c_str(),
        true
    );
    (void)materials;
    if (!parsed) {
        std::cerr << "[ObjLoader] Failed to parse OBJ '" << path.string() << "': " << err << std::endl;
        return false;
    }
    if (!err.empty()) {
        std::cerr << "[ObjLoader] Warning for '" << path.string() << "': " << err << std::endl;
    }

    if ((attrib.vertices.size() % 3U) != 0U) {
        std::cerr << "[ObjLoader] Vertex buffer is malformed in '" << path.string() << "'." << std::endl;
        return false;
    }
    const size_t sourceVertexCount = attrib.vertices.size() / 3U;

    size_t totalIndexCount = 0;
    for (const tinyobj::shape_t& shape : shapes) {
        if ((shape.mesh.indices.size() % 3U) != 0U) {
            std::cerr << "[ObjLoader] Non-triangle indices detected in '" << path.string() << "'." <<
                std::endl;
            return false;
        }
        totalIndexCount += shape.mesh.indices.size();
    }
    if (totalIndexCount > static_cast<size_t>(std::numeric_limits<uint16_t>::max())) {
        std::cerr << "[ObjLoader] Expanded face-vertex count exceeds uint16_t range in '" << path.string() <<
            "'." << std::endl;
        return false;
    }

    outMesh.vertices.reserve(totalIndexCount);
    outMesh.indices.reserve(totalIndexCount);

    uint32_t faceIndex = 0;
    for (const tinyobj::shape_t& shape : shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3U) {
            const glm::vec3 faceColor = FaceColorFromIndex(faceIndex++);
            for (size_t corner = 0; corner < 3U; ++corner) {
                const tinyobj::index_t& idx = shape.mesh.indices[i + corner];
                if (idx.vertex_index < 0) {
                    std::cerr << "[ObjLoader] Found negative vertex index in '" << path.string() << "'." <<
                        std::endl;
                    outMesh.vertices.clear();
                    outMesh.indices.clear();
                    return false;
                }

                const uint32_t vertexIndex = static_cast<uint32_t>(idx.vertex_index);
                if (vertexIndex >= sourceVertexCount) {
                    std::cerr << "[ObjLoader] Vertex index out of range in '" << path.string() << "'." <<
                        std::endl;
                    outMesh.vertices.clear();
                    outMesh.indices.clear();
                    return false;
                }

                if (outMesh.vertices.size() > static_cast<size_t>(std::numeric_limits<uint16_t>::max())) {
                    std::cerr << "[ObjLoader] Expanded mesh vertex index exceeds uint16_t range in '" <<
                        path.string() << "'." << std::endl;
                    outMesh.vertices.clear();
                    outMesh.indices.clear();
                    return false;
                }

                const size_t positionBase = static_cast<size_t>(vertexIndex) * 3U;
                Vertex3D     vertex{};
                vertex.position = {
                    attrib.vertices[positionBase + 0U],
                    attrib.vertices[positionBase + 1U],
                    attrib.vertices[positionBase + 2U]
                };
                vertex.color = faceColor;

                outMesh.indices.push_back(static_cast<uint16_t>(outMesh.vertices.size()));
                outMesh.vertices.push_back(vertex);
            }
        }
    }

    if (outMesh.vertices.empty() || outMesh.indices.empty()) {
        std::cerr << "[ObjLoader] Parsed empty mesh from '" << path.string() << "'." << std::endl;
        outMesh.vertices.clear();
        outMesh.indices.clear();
        return false;
    }

    return true;
}
