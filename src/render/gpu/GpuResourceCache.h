#ifndef BJTU_WGPU_RENDERER_GPURESOURCECACHE_H
#define BJTU_WGPU_RENDERER_GPURESOURCECACHE_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "GpuMesh.h"
#include "render/scene/RenderObject.h"

class RenderContext;

class GpuResourceCache {
public:
    [[nodiscard]] const GpuMesh* SyncMesh(RenderContext& ctx, const RenderObject& object);

    void Reset();

private:
    struct CacheKey {
        const LegacyMeshData3D* mesh = nullptr;
        Object3D::ERenderMode   renderMode = Object3D::ERenderMode::Solid;

        bool operator==(const CacheKey& other) const = default;
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& key) const;
    };

    static GpuMesh UploadMeshToGpu(RenderContext& ctx, const RenderObject& object);

    std::unordered_map<CacheKey, GpuMesh, CacheKeyHash> m_meshes;
};

#endif // BJTU_WGPU_RENDERER_GPURESOURCECACHE_H
