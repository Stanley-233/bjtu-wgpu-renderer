#include "LegacyResourceManager.h"

#include "resource/legacy/LegacyTomlSceneLoader.h"
#include "resource/loaders/LegacyTxtGeometryLoader.h"
#include "resource/loaders/LegacyObjLoader.h"

bool LegacyResourceManager::LoadGeometry(
    const std::filesystem::path& path,
    std::vector<float>&          pointData,
    std::vector<uint16_t>&       indexData
) {
    constexpr LegacyTxtGeometryLoader loader{};
    return loader.Load(path, pointData, indexData);
}

bool LegacyResourceManager::LoadGeometry3DFromObj(
    const std::filesystem::path& path,
    LegacyMeshData3D&            outMesh
) {
    LegacyObjLoader loader{};
    return loader.Load(path, outMesh);
}

bool LegacyResourceManager::LoadSceneFromToml(
    const std::filesystem::path& path,
    SceneDescription&            outScene
) {
    LegacyTomlSceneLoader loader{};
    return loader.Load(path, outScene);
}
