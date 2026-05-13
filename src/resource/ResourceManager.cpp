#include "ResourceManager.h"

#include "loaders/LegacyTxtGeometryLoader.h"
#include "loaders/ObjLoader.h"
#include "loaders/ShaderLoader.h"
#include "legacy/LegacyTomlSceneLoader.h"

bool ResourceManager::LoadGeometry(
    const std::filesystem::path& path,
    std::vector<float>&          pointData,
    std::vector<uint16_t>&       indexData
) {
    constexpr LegacyTxtGeometryLoader loader{};
    return loader.Load(path, pointData, indexData);
}

bool ResourceManager::LoadGeometry3DFromObj(
    const std::filesystem::path& path,
    LegacyMeshData3D&                  outMesh
) {
    ObjLoader loader{};
    return loader.Load(path, outMesh);
}

bool ResourceManager::LoadSceneFromToml(
    const std::filesystem::path& path,
    SceneDescription&            outScene
) {
    LegacyTomlSceneLoader loader{};
    return loader.Load(path, outScene);
}

wgpu::ShaderModule ResourceManager::LoadShaderModule(const std::filesystem::path& path, const wgpu::Device device) {
    constexpr ShaderLoader loader{};
    return loader.Load(path, device);
}
