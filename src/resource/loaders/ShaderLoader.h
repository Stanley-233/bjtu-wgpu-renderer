#ifndef BJTU_WGPU_RENDERER_SHADERLOADER_H
#define BJTU_WGPU_RENDERER_SHADERLOADER_H

#include <filesystem>

#include <webgpu/webgpu.hpp>

class ShaderLoader {
public:
    static wgpu::ShaderModule Load(const std::filesystem::path& path, wgpu::Device device);
};

#endif // BJTU_WGPU_RENDERER_SHADERLOADER_H
