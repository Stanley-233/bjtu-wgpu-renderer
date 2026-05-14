#include "GltfImporter.h"

bool GltfImporter::Import(const std::filesystem::path& path, ModelAsset& outModel) const {
    (void)path;
    outModel = ModelAsset{};
    return false;
}
