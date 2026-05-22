#include "EnvironmentMapCache.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <glm/packing.hpp>

#include <stb_image.h>

#include "render/RenderContext.h"

namespace {

struct HdrImageData {
    int                width = 0;
    int                height = 0;
    std::vector<float> pixels{};
};

[[nodiscard]] HdrImageData LoadHdrImage(const std::filesystem::path& path) {
    HdrImageData image{};
    int channels = 0;
    float* pixels = stbi_loadf(path.string().c_str(), &image.width, &image.height, &channels, 4);
    if (pixels == nullptr) {
        std::cerr << "[EnvironmentMapCache] Failed to load HDR image: " << path.string()
                  << " (" << stbi_failure_reason() << ")" << std::endl;
        return image;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U;
    image.pixels.assign(pixels, pixels + pixelCount);
    stbi_image_free(pixels);
    return image;
}

[[nodiscard]] std::vector<uint16_t> ConvertFloatPixelsToHalf(const std::vector<float>& pixels) {
    std::vector<uint16_t> halfPixels{};
    halfPixels.resize(pixels.size());
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
        const uint32_t packed01 = glm::packHalf2x16(glm::vec2{
            pixels[i + 0],
            pixels[i + 1],
        });
        const uint32_t packed23 = glm::packHalf2x16(glm::vec2{
            pixels[i + 2],
            pixels[i + 3],
        });
        std::memcpy(halfPixels.data() + i + 0, &packed01, sizeof(packed01));
        std::memcpy(halfPixels.data() + i + 2, &packed23, sizeof(packed23));
    }
    return halfPixels;
}

[[nodiscard]] wgpu::raii::Sampler CreateLinearClampSampler(RenderContext& renderCtx) {
    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    samplerDesc.maxAnisotropy = 1;
    return renderCtx.GetDevice()->createSampler(samplerDesc);
}

} // namespace

std::string EnvironmentMapCache::BuildCacheKey(const std::filesystem::path& hdrPath, const uint32_t faceSize) {
    return hdrPath.lexically_normal().generic_string() + "#" + std::to_string(faceSize);
}

const EnvironmentMapGpuResources* EnvironmentMapCache::GetOrCreate(
    RenderContext&               renderCtx,
    const std::filesystem::path& hdrPath,
    const uint32_t               faceSize) {
    const std::string key = BuildCacheKey(hdrPath, faceSize);
    auto [it, inserted] = m_entries.try_emplace(key);
    CacheEntry& entry = it->second;
    if (entry.failed) {
        return nullptr;
    }
    if (!inserted && entry.resources.cubemapCubeView) {
        return &entry.resources;
    }

    if (!LoadEnvironmentMap(renderCtx, hdrPath, faceSize, entry.resources)) {
        entry.failed = true;
        return nullptr;
    }
    return &entry.resources;
}

void EnvironmentMapCache::Reset() {
    m_entries.clear();
    m_computePipeline.reset();
}

void EnvironmentMapCache::EnsureComputePipeline(RenderContext& renderCtx) {
    if (!m_computePipeline.has_value()) {
        m_computePipeline = Scene3DPipelineFactory::CreateEquirectToCubemapComputePipeline(renderCtx);
    }
}

bool EnvironmentMapCache::LoadEnvironmentMap(
    RenderContext&               renderCtx,
    const std::filesystem::path& hdrPath,
    const uint32_t               faceSize,
    EnvironmentMapGpuResources&  outResources) {
    const HdrImageData hdrImage = LoadHdrImage(hdrPath);
    if (hdrImage.width <= 0 || hdrImage.height <= 0 || hdrImage.pixels.empty() || faceSize == 0) {
        return false;
    }

    const std::vector<uint16_t> halfPixels = ConvertFloatPixelsToHalf(hdrImage.pixels);
    if (halfPixels.empty()) {
        return false;
    }

    outResources = {};
    outResources.faceSize = faceSize;
    outResources.sampler = CreateLinearClampSampler(renderCtx);

    wgpu::TextureDescriptor equirectDesc{};
    equirectDesc.dimension = wgpu::TextureDimension::_2D;
    equirectDesc.size.width = static_cast<uint32_t>(hdrImage.width);
    equirectDesc.size.height = static_cast<uint32_t>(hdrImage.height);
    equirectDesc.size.depthOrArrayLayers = 1;
    equirectDesc.sampleCount = 1;
    equirectDesc.mipLevelCount = 1;
    equirectDesc.format = wgpu::TextureFormat::RGBA16Float;
    equirectDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
    outResources.equirectTexture = renderCtx.GetDevice()->createTexture(equirectDesc);
    outResources.equirectView = outResources.equirectTexture->createView();

    WGPUImageCopyTexture dstView{};
    dstView.texture = *outResources.equirectTexture;
    dstView.mipLevel = 0;
    dstView.origin = {0, 0, 0};
    dstView.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout layout{};
    layout.offset = 0;
    layout.bytesPerRow = static_cast<uint32_t>(hdrImage.width) * 4U * sizeof(uint16_t);
    layout.rowsPerImage = static_cast<uint32_t>(hdrImage.height);

    WGPUExtent3D size{
        .width = static_cast<uint32_t>(hdrImage.width),
        .height = static_cast<uint32_t>(hdrImage.height),
        .depthOrArrayLayers = 1,
    };
    wgpuQueueWriteTexture(
        *renderCtx.GetQueue(),
        &dstView,
        halfPixels.data(),
        static_cast<uint32_t>(halfPixels.size() * sizeof(uint16_t)),
        &layout,
        &size);

    wgpu::TextureDescriptor cubemapDesc{};
    cubemapDesc.dimension = wgpu::TextureDimension::_2D;
    cubemapDesc.size.width = faceSize;
    cubemapDesc.size.height = faceSize;
    cubemapDesc.size.depthOrArrayLayers = 6;
    cubemapDesc.sampleCount = 1;
    cubemapDesc.mipLevelCount = 1;
    cubemapDesc.format = wgpu::TextureFormat::RGBA16Float;
    cubemapDesc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
    outResources.cubemapTexture = renderCtx.GetDevice()->createTexture(cubemapDesc);

    wgpu::TextureViewDescriptor arrayViewDesc{};
    arrayViewDesc.dimension = wgpu::TextureViewDimension::_2DArray;
    arrayViewDesc.format = cubemapDesc.format;
    arrayViewDesc.baseMipLevel = 0;
    arrayViewDesc.mipLevelCount = 1;
    arrayViewDesc.baseArrayLayer = 0;
    arrayViewDesc.arrayLayerCount = 6;
    arrayViewDesc.aspect = wgpu::TextureAspect::All;
    outResources.cubemapArrayView = outResources.cubemapTexture->createView(arrayViewDesc);

    wgpu::TextureViewDescriptor cubeViewDesc{};
    cubeViewDesc.dimension = wgpu::TextureViewDimension::Cube;
    cubeViewDesc.format = cubemapDesc.format;
    cubeViewDesc.baseMipLevel = 0;
    cubeViewDesc.mipLevelCount = 1;
    cubeViewDesc.baseArrayLayer = 0;
    cubeViewDesc.arrayLayerCount = 6;
    cubeViewDesc.aspect = wgpu::TextureAspect::All;
    outResources.cubemapCubeView = outResources.cubemapTexture->createView(cubeViewDesc);

    EnsureComputePipeline(renderCtx);
    if (!m_computePipeline.has_value()
        || !m_computePipeline->bindGroupLayout
        || !m_computePipeline->pipeline
        || !outResources.equirectView
        || !outResources.cubemapArrayView
        || !outResources.sampler) {
        std::cerr << "[EnvironmentMapCache] Compute pipeline was not ready for " << hdrPath.string() << std::endl;
        return false;
    }

    wgpu::BindGroupEntry bindings[3]{};
    bindings[0].binding = 0;
    bindings[0].textureView = *outResources.equirectView;
    bindings[1].binding = 1;
    bindings[1].sampler = *outResources.sampler;
    bindings[2].binding = 2;
    bindings[2].textureView = *outResources.cubemapArrayView;

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_computePipeline->bindGroupLayout;
    bindGroupDesc.entryCount = 3;
    bindGroupDesc.entries = bindings;
    wgpu::raii::BindGroup bindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);
    if (!bindGroup) {
        std::cerr << "[EnvironmentMapCache] Failed to create compute bind group for " << hdrPath.string() << std::endl;
        return false;
    }

    wgpu::raii::CommandEncoder encoder = renderCtx.CreateCommandEncoder();
    wgpu::ComputePassDescriptor computePassDesc{};
    wgpu::raii::ComputePassEncoder computePass = encoder->beginComputePass(computePassDesc);
    computePass->setPipeline(*m_computePipeline->pipeline);
    computePass->setBindGroup(0, *bindGroup, 0, nullptr);
    const uint32_t workgroupCount = (faceSize + 7U) / 8U;
    computePass->dispatchWorkgroups(workgroupCount, workgroupCount, 6U);
    computePass->end();
    renderCtx.Submit(encoder);

    return true;
}
