#pragma once

#include <filesystem>

#include <webgpu/webgpu.hpp>

class LegacyShaderLoader {
public:
    static wgpu::ShaderModule Load(const std::filesystem::path& path, wgpu::Device device);
};
