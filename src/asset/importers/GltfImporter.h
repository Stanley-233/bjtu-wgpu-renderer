#pragma once

#include <filesystem>

#include "asset/types/ModelAsset.h"

class GltfImporter {
public:
    bool Import(const std::filesystem::path& path, ModelAsset& outModel) const;
};
