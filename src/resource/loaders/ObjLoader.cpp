#include "ObjLoader.h"

bool ObjLoader::Load(const std::filesystem::path& path, MeshData3D& outMesh) {
    // TODO: 这里需要真正实现 OBJ 文件解析流程（读取顶点/索引并转换为 MeshData3D）
    (void)path;
    outMesh.vertices.clear();
    outMesh.indices.clear();
    // TODO: 当前仅为占位返回 false；完成解析后应在成功时返回 true
    return false;
}
