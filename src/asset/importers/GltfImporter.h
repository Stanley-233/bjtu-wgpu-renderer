#pragma once

#include <filesystem>

#include "asset/types/ModelAsset.h"

class AssetServer;

class GltfImporter {
public:
    static bool Import(const std::filesystem::path& path, AssetServer& assetServer, ModelAsset& outModel);
};
