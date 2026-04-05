#ifndef BJTU_WGPU_RENDERER_ISCENELOADER_H
#define BJTU_WGPU_RENDERER_ISCENELOADER_H

#include <filesystem>

#include "../models/SceneDescription.h"

class ISceneLoader {
public:
    virtual ~ISceneLoader() = default;

    virtual bool Load(const std::filesystem::path& path, SceneDescription& outScene) = 0;
};

#endif // BJTU_WGPU_RENDERER_ISCENELOADER_H
