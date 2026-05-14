#pragma once

#include <filesystem>

namespace AssetPaths {

std::filesystem::path AssetRoot();
std::filesystem::path Resolve(const std::filesystem::path& relativePath);

} // namespace AssetPaths
