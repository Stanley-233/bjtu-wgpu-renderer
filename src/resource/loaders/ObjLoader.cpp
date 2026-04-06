#include "ObjLoader.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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

    constexpr glm::vec3 kTeapotColor{0.86f, 0.87f, 0.78f};

    if ((attrib.vertices.size() % 3U) != 0U) {
        std::cerr << "[ObjLoader] Vertex buffer is malformed in '" << path.string() << "'." << std::endl;
        return false;
    }
    if ((attrib.vertices.size() / 3U) > static_cast<size_t>(std::numeric_limits<uint16_t>::max())) {
        std::cerr << "[ObjLoader] Vertex count exceeds uint16_t range in '" << path.string() << "'." <<
            std::endl;
        return false;
    }

    outMesh.vertices.reserve(attrib.vertices.size() / 3U);
    for (size_t vertexIdx = 0; vertexIdx < attrib.vertices.size(); vertexIdx += 3U) {
        Vertex3D vertex{};
        vertex.position = {
            attrib.vertices[vertexIdx + 0U],
            attrib.vertices[vertexIdx + 1U],
            attrib.vertices[vertexIdx + 2U]
        };
        vertex.color = kTeapotColor;
        outMesh.vertices.push_back(vertex);
    }

    size_t totalIndexCount = 0;
    for (const tinyobj::shape_t& shape : shapes) {
        totalIndexCount += shape.mesh.indices.size();
    }
    outMesh.indices.reserve(totalIndexCount);

    for (const tinyobj::shape_t& shape : shapes) {
        for (const tinyobj::index_t& idx : shape.mesh.indices) {
            if (idx.vertex_index < 0) {
                std::cerr << "[ObjLoader] Found negative vertex index in '" << path.string() << "'." <<
                    std::endl;
                outMesh.vertices.clear();
                outMesh.indices.clear();
                return false;
            }
            const uint32_t vertexIndex = static_cast<uint32_t>(idx.vertex_index);
            if (vertexIndex >= outMesh.vertices.size()) {
                std::cerr << "[ObjLoader] Vertex index out of range in '" << path.string() << "'." <<
                    std::endl;
                outMesh.vertices.clear();
                outMesh.indices.clear();
                return false;
            }
            outMesh.indices.push_back(static_cast<uint16_t>(vertexIndex));
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