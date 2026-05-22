#include "HdrImageImporter.h"

#include <iostream>

#include <stb_image.h>

bool HdrImageImporter::Import(const std::filesystem::path& path, HdrImageAsset& outAsset) {
    int width = 0;
    int height = 0;
    int channels = 0;
    float* pixels = stbi_loadf(path.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        std::cerr << "[HdrImageImporter] Failed to load HDR image: " << path.string()
                  << " (" << stbi_failure_reason() << ")" << std::endl;
        return false;
    }

    outAsset = {};
    outAsset.sourcePath = path;
    outAsset.width = static_cast<uint32_t>(width);
    outAsset.height = static_cast<uint32_t>(height);
    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    outAsset.pixels.assign(pixels, pixels + pixelCount);
    stbi_image_free(pixels);
    return true;
}
