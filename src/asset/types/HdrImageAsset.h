#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct HdrImageAsset {
    std::filesystem::path sourcePath{};
    uint32_t              width = 0;
    uint32_t              height = 0;
    std::vector<float>    pixels{};
};
