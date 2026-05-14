#pragma once

#include <filesystem>

#include "asset/types/ModelAsset.h"

class AssetServer;

class GltfImporter {
public:
    bool Import(const std::filesystem::path& path, AssetServer& assetServer, ModelAsset& outModel) const;
};
