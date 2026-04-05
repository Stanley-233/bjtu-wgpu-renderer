#include "TomlSceneLoader.h"

bool TomlSceneLoader::Load(const std::filesystem::path& path, SceneDescription& outScene) {
    (void)path;
    outScene = SceneDescription{};
    return false;
}
