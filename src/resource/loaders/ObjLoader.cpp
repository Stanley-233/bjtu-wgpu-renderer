#include "ObjLoader.h"

bool ObjLoader::Load(const std::filesystem::path& path, MeshData3D& outMesh) {
    (void)path;
    outMesh.vertices.clear();
    outMesh.indices.clear();
    return false;
}
