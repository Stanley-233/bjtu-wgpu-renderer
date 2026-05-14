#pragma once

#include <filesystem>

namespace LegacyResourcePaths {

std::filesystem::path LegacyAssetsRoot();
std::filesystem::path LegacyShadersRoot();
std::filesystem::path ResolveAsset(const std::filesystem::path& relativePath);
std::filesystem::path ResolveShader(const std::filesystem::path& relativePath);

} // namespace LegacyResourcePaths
