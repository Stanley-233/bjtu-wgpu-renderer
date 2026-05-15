#pragma once

#include <filesystem>

namespace ShaderPaths {

std::filesystem::path ShaderRoot();
std::filesystem::path Resolve(const std::filesystem::path& relativePath);

} // namespace ShaderPaths
