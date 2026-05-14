#pragma once

#include <filesystem>

namespace ConfigPaths {

std::filesystem::path ConfigRoot();
std::filesystem::path Resolve(const std::filesystem::path& relativePath);

} // namespace ConfigPaths
