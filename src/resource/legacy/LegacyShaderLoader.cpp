#include "LegacyShaderLoader.h"

#include <fstream>
#include <string>

wgpu::ShaderModule LegacyShaderLoader::Load(const std::filesystem::path& path, wgpu::Device device) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    const size_t size = static_cast<size_t>(file.tellg());
    std::string  shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), static_cast<std::streamsize>(size));

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next  = nullptr;
    shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code        = shaderSource.c_str();

    wgpu::ShaderModuleDescriptor shaderDesc{};
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints     = nullptr;
#endif
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return device.createShaderModule(shaderDesc);
}
