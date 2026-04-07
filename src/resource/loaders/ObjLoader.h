#ifndef BJTU_WGPU_RENDERER_OBJLOADER_H
#define BJTU_WGPU_RENDERER_OBJLOADER_H

#include "IModelLoader.h"

class ObjLoader final : public IModelLoader {
public:
    bool Load(const std::filesystem::path& path, MeshData3D& outMesh) override;
};

#endif // BJTU_WGPU_RENDERER_OBJLOADER_H
