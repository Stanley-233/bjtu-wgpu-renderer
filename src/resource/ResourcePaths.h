#pragma once

#include <filesystem>

namespace ResourcePaths {

std::filesystem::path ResourceRoot();
std::filesystem::path Resolve(const std::filesystem::path& relativePath);

} // namespace ResourcePaths
