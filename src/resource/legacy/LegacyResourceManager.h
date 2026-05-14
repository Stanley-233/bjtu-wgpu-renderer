#pragma once

#include <filesystem>
#include <vector>

struct LegacyMeshData3D;
struct SceneDescription;

class LegacyResourceManager {
public:
    static bool LoadGeometry(
        const std::filesystem::path& path,
        std::vector<float>&          pointData,
        std::vector<uint16_t>&       indexData
    );

    static bool LoadGeometry3DFromObj(
        const std::filesystem::path& path,
        LegacyMeshData3D&            outMesh
    );

    static bool LoadSceneFromToml(
        const std::filesystem::path& path,
        SceneDescription&            outScene
    );
};
