#ifndef BJTU_WGPU_RENDERER_TOMLSCENELOADER_H
#define BJTU_WGPU_RENDERER_TOMLSCENELOADER_H

#include "resource/loaders/ISceneLoader.h"

class LegacyTomlSceneLoader final : public ISceneLoader {
public:
    bool Load(const std::filesystem::path& path, SceneDescription& outScene) override;
};

#endif // BJTU_WGPU_RENDERER_TOMLSCENELOADER_H
