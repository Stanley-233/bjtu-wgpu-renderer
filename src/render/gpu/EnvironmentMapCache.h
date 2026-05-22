#ifndef BJTU_WGPU_RENDERER_ENVIRONMENTMAPCACHE_H
#define BJTU_WGPU_RENDERER_ENVIRONMENTMAPCACHE_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include "asset/types/HdrImageAsset.h"
#include "render/pipelines/Scene3DPipelineFactory.h"
#include "webgpu-raii.hpp"

class RenderContext;

struct EnvironmentMapGpuResources {
    wgpu::raii::Texture     equirectTexture;
    wgpu::raii::TextureView equirectView;
    wgpu::raii::Texture     cubemapTexture;
    wgpu::raii::TextureView cubemapArrayView;
    wgpu::raii::TextureView cubemapCubeView;
    wgpu::raii::Sampler     sampler;
    uint32_t                faceSize = 0;
};

class EnvironmentMapCache {
public:
    [[nodiscard]] const EnvironmentMapGpuResources* GetOrCreate(
        RenderContext&               renderCtx,
        const HdrImageAsset&         hdrImage,
        uint32_t                     faceSize);

    void Reset();

private:
    struct CacheEntry {
        EnvironmentMapGpuResources resources{};
        bool                       failed = false;
    };

    [[nodiscard]] static std::string BuildCacheKey(const HdrImageAsset& hdrImage, uint32_t faceSize);

    void EnsureComputePipeline(RenderContext& renderCtx);

    [[nodiscard]] bool LoadEnvironmentMap(
        RenderContext&               renderCtx,
        const HdrImageAsset&         hdrImage,
        uint32_t                     faceSize,
        EnvironmentMapGpuResources&  outResources);

    std::unordered_map<std::string, CacheEntry>                              m_entries;
    std::optional<Scene3DPipelineFactory::EquirectToCubemapComputePipeline>  m_computePipeline;
};

#endif // BJTU_WGPU_RENDERER_ENVIRONMENTMAPCACHE_H
