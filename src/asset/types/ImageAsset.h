#pragma once

#include <cstdint>
#include <vector>

enum class EImageAssetFormat {
    Rgba8Srgb,
    Rgba8Unorm,
};

struct ImageAsset {
    uint32_t                     width = 0;
    uint32_t                     height = 0;
    EImageAssetFormat             format = EImageAssetFormat::Rgba8Srgb;
    std::vector<std::uint8_t>    pixels;
};
