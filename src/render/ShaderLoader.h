#pragma once

#include <filesystem>

#include <webgpu/webgpu.hpp>

class ShaderLoader {
public:
    static wgpu::ShaderModule Load(const std::filesystem::path& path, wgpu::Device device);
};
