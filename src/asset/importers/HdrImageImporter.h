#pragma once

#include <filesystem>

#include "asset/types/HdrImageAsset.h"

class HdrImageImporter {
public:
    static bool Import(const std::filesystem::path& path, HdrImageAsset& outAsset);
};
