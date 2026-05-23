#include "ShaderLoader.h"

#include <fstream>
#include <iterator>
#include <string>

wgpu::ShaderModule ShaderLoader::Load(const std::filesystem::path& path, wgpu::Device device) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }

    std::string shaderSource{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    // Strip UTF-8 BOM if present. Some WGSL parsers reject it as source text.
    if (shaderSource.size() >= 3
        && static_cast<unsigned char>(shaderSource[0]) == 0xEF
        && static_cast<unsigned char>(shaderSource[1]) == 0xBB
        && static_cast<unsigned char>(shaderSource[2]) == 0xBF) {
        shaderSource.erase(0, 3);
    }

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shaderSource.c_str();

    wgpu::ShaderModuleDescriptor shaderDesc{};
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return device.createShaderModule(shaderDesc);
}
