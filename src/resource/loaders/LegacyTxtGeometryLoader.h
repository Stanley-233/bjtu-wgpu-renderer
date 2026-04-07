#ifndef BJTU_WGPU_RENDERER_LEGACYTXTGEOMETRYLOADER_H
#define BJTU_WGPU_RENDERER_LEGACYTXTGEOMETRYLOADER_H

#include <filesystem>
#include <vector>

class LegacyTxtGeometryLoader {
public:
    static bool Load(
        const std::filesystem::path& path,
        std::vector<float>&          pointData,
        std::vector<uint16_t>&       indexData
    );
};

#endif // BJTU_WGPU_RENDERER_LEGACYTXTGEOMETRYLOADER_H
