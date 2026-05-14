#ifndef BJTU_WGPU_RENDERER_OBJLOADER_H
#define BJTU_WGPU_RENDERER_OBJLOADER_H

#include "resource/legacy/LegacyMeshData3D.h"

#include <filesystem>

class LegacyObjLoader final {
public:
    bool Load(const std::filesystem::path& path, LegacyMeshData3D& outMesh);
};

#endif // BJTU_WGPU_RENDERER_OBJLOADER_H
