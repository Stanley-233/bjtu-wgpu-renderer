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
#include "render/scene/RenderUniformData.h"

class RenderContext;

class GpuResourceCache {
public:
    [[nodiscard]] const GpuMesh* SyncMesh(RenderContext& renderCtx, const AssetServer* assetServer, const RenderObject& object);

    struct GpuMaterialResources {
        MaterialUniformData uniformData{};
        wgpu::raii::Buffer  uniformBuffer;
        wgpu::TextureView   baseColorTextureView = nullptr;
        wgpu::TextureView   normalTextureView = nullptr;
        wgpu::TextureView   metallicRoughnessTextureView = nullptr;
        wgpu::Sampler       sampler = nullptr;
    };

    [[nodiscard]] const GpuMaterialResources* SyncMaterial(
        RenderContext& renderCtx,
        const AssetServer* assetServer,
        const RenderObject& object);

    void Reset();

private:
    struct GpuTexture2D {
        wgpu::raii::Texture     texture;
        wgpu::raii::TextureView view;
    };

    struct MaterialCacheKey {
        AssetId<MaterialAsset> materialId{};
        EMaterialShadingModel  shadingModel{EMaterialShadingModel::Unlit};

        bool operator==(const MaterialCacheKey& other) const = default;
    };

    struct CacheKey {
        AssetId<MeshAsset>      meshId{};
        const LegacyMeshData3D* legacyMesh = nullptr;

        bool operator==(const CacheKey& other) const = default;
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& key) const;
    };

    struct MaterialCacheKeyHash {
        std::size_t operator()(const MaterialCacheKey& key) const;
    };

    struct AssetIdHash {
        template <typename T>
        std::size_t operator()(const AssetId<T> key) const {
            return key.value;
        }
    };

    static GpuMesh UploadMeshToGpu(RenderContext& renderCtx, const MeshAsset* assetMesh, const LegacyMeshData3D* legacyMesh);
    static GpuTexture2D UploadTextureToGpu(RenderContext& renderCtx, const ImageAsset& image);

    [[nodiscard]] const GpuTexture2D* SyncTexture(
        RenderContext& renderCtx,
        const AssetServer* assetServer,
        AssetId<ImageAsset> imageId);

    [[nodiscard]] const GpuTexture2D* GetOrCreateWhiteTexture(RenderContext& renderCtx);

    [[nodiscard]] const GpuTexture2D* GetOrCreateLinearWhiteTexture(RenderContext& renderCtx);

    [[nodiscard]] const GpuTexture2D* GetOrCreateFlatNormalTexture(RenderContext& renderCtx);

    [[nodiscard]] GpuMaterialResources BuildMaterialResources(
        RenderContext& renderCtx,
        const MaterialAsset& material,
        EMaterialShadingModel shadingModel,
        const GpuTexture2D& baseColorTexture,
        const GpuTexture2D& normalTexture,
        const GpuTexture2D& metallicRoughnessTexture) const;

    std::unordered_map<CacheKey, GpuMesh, CacheKeyHash> m_meshes;
    std::unordered_map<AssetId<ImageAsset>, GpuTexture2D, AssetIdHash> m_textures;
    std::unordered_map<MaterialCacheKey, GpuMaterialResources, MaterialCacheKeyHash> m_materials;
    std::optional<GpuTexture2D> m_whiteTexture;
    std::optional<GpuTexture2D> m_linearWhiteTexture;
    std::optional<GpuTexture2D> m_flatNormalTexture;
    wgpu::raii::Sampler m_sampler;
};

#endif // BJTU_WGPU_RENDERER_GPURESOURCECACHE_H
