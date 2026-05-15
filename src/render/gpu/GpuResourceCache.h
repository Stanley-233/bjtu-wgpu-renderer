#ifndef BJTU_WGPU_RENDERER_GPURESOURCECACHE_H
#define BJTU_WGPU_RENDERER_GPURESOURCECACHE_H

#include <cstdint>
#include <optional>
#include <unordered_map>

#include <glm/vec4.hpp>

#include "asset/AssetId.h"
#include "asset/AssetServer.h"
#include "asset/types/ImageAsset.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"
#include "GpuMesh.h"
#include "render/scene/RenderObject.h"

class RenderContext;

class GpuResourceCache {
public:
    [[nodiscard]] const GpuMesh* SyncMesh(RenderContext& ctx, const RenderObject& object);

    struct GpuMaterialResources {
        struct alignas(16) MaterialUniformData {
            glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        };

        MaterialUniformData uniformData{};
        wgpu::raii::Buffer  uniformBuffer;
        wgpu::TextureView   textureView = nullptr;
        wgpu::Sampler       sampler = nullptr;
    };

    [[nodiscard]] const GpuMaterialResources* SyncMaterial(
        RenderContext& ctx,
        const AssetServer* assetServer,
        const RenderObject& object);

    void Reset();

private:
    struct GpuTexture2D {
        wgpu::raii::Texture     texture;
        wgpu::raii::TextureView view;
    };

    struct CacheKey {
        AssetId<MeshAsset>      meshId{};
        const LegacyMeshData3D* legacyMesh = nullptr;

        bool operator==(const CacheKey& other) const = default;
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& key) const;
    };

    struct AssetIdHash {
        template <typename T>
        std::size_t operator()(const AssetId<T> key) const {
            return key.value;
        }
    };

    static GpuMesh UploadMeshToGpu(RenderContext& ctx, const RenderObject& object);
    static GpuTexture2D UploadTextureToGpu(RenderContext& ctx, const ImageAsset& image);

    [[nodiscard]] const GpuTexture2D* SyncTexture(
        RenderContext& ctx,
        const AssetServer* assetServer,
        AssetId<ImageAsset> imageId);

    [[nodiscard]] const GpuTexture2D* GetOrCreateWhiteTexture(RenderContext& ctx);

    [[nodiscard]] GpuMaterialResources BuildMaterialResources(
        RenderContext& ctx,
        const MaterialAsset& material,
        const GpuTexture2D& baseColorTexture) const;

    std::unordered_map<CacheKey, GpuMesh, CacheKeyHash>                  m_meshes;
    std::unordered_map<AssetId<ImageAsset>, GpuTexture2D, AssetIdHash>   m_textures;
    std::unordered_map<AssetId<MaterialAsset>, GpuMaterialResources, AssetIdHash> m_materials;
    std::optional<GpuTexture2D>                                           m_whiteTexture;
    std::optional<GpuMaterialResources>                                   m_defaultMaterial;
    wgpu::raii::Sampler                                                   m_sampler;
};

#endif // BJTU_WGPU_RENDERER_GPURESOURCECACHE_H
