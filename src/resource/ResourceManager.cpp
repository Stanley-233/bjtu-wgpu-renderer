#include "ResourceManager.h"

#include "loaders/LegacyTxtGeometryLoader.h"
#include "loaders/ObjLoader.h"
#include "loaders/ShaderLoader.h"
#include "loaders/TomlSceneLoader.h"
#include "models/MeshData3D.h"
#include "models/SceneDescription.h"

bool ResourceManager::LoadGeometry(
    const std::filesystem::path& path,
    std::vector<float>&          pointData,
    std::vector<uint16_t>&       indexData
) {
    const LegacyTxtGeometryLoader loader{};
    return loader.Load(path, pointData, indexData);
}

bool ResourceManager::LoadGeometry3DFromObj(
    const std::filesystem::path& path,
    MeshData3D&                  outMesh
) {
    ObjLoader loader{};
    return loader.Load(path, outMesh);
}

bool ResourceManager::LoadSceneFromToml(
    const std::filesystem::path& path,
    SceneDescription&            outScene
) {
    TomlSceneLoader loader{};
    return loader.Load(path, outScene);
}

wgpu::ShaderModule ResourceManager::LoadShaderModule(const std::filesystem::path& path, const wgpu::Device device) {
    const ShaderLoader loader{};
    return loader.Load(path, device);
}
