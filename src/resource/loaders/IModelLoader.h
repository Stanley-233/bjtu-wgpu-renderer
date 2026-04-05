#ifndef BJTU_WGPU_RENDERER_IMODELLOADER_H
#define BJTU_WGPU_RENDERER_IMODELLOADER_H

#include <filesystem>

#include "../models/MeshData3D.h"

class IModelLoader {
public:
    virtual ~IModelLoader() = default;

    virtual bool Load(const std::filesystem::path& path, MeshData3D& outMesh) = 0;
};

#endif // BJTU_WGPU_RENDERER_IMODELLOADER_H
