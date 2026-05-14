#include "GpuResourceCache.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "render/RenderContext.h"

std::size_t GpuResourceCache::CacheKeyHash::operator()(const CacheKey& key) const {
    const auto meshBits = key.meshId.IsValid()
                              ? static_cast<std::uintptr_t>(key.meshId.value)
                              : reinterpret_cast<std::uintptr_t>(key.legacyMesh);
    const auto modeBits = static_cast<std::size_t>(key.renderMode == Object3D::ERenderMode::Wireframe ? 1U : 0U);
    return meshBits ^ (modeBits << 1U);
}

const GpuMesh* GpuResourceCache::SyncMesh(RenderContext& ctx, const RenderObject& object) {
    const MeshAsset* const assetMesh = object.mesh;
    const LegacyMeshData3D* const legacyMesh = object.legacyMesh;
    const bool hasAssetMesh = assetMesh != nullptr && !assetMesh->vertices.empty() && !assetMesh->indices.empty();
    const bool hasLegacyMesh = legacyMesh != nullptr && !legacyMesh->vertices.empty() && !legacyMesh->indices.empty();
    if (!hasAssetMesh && !hasLegacyMesh) {
        return nullptr;
    }

    const CacheKey key{
        .meshId = object.meshId,
        .legacyMesh = object.meshId.IsValid() ? nullptr : object.legacyMesh,
        .renderMode = object.renderMode,
    };
    auto [it, inserted] = m_meshes.try_emplace(key);
    GpuMesh& mesh = it->second;

    const auto& sourceVertices = hasAssetMesh ? assetMesh->vertices : legacyMesh->vertices;
    const auto& sourceIndices = hasAssetMesh ? assetMesh->indices : legacyMesh->indices;
    const uint64_t vertexBytes = sourceVertices.size() * sizeof(Vertex3D);
    const uint32_t sourceVertexCount = static_cast<uint32_t>(sourceVertices.size());
    const uint32_t sourceIndexCount = static_cast<uint32_t>(sourceIndices.size());
    const bool needsUpload = inserted
                             || !mesh.vertexBuffer
                             || !mesh.indexBuffer
                             || mesh.vertexBufferSize != vertexBytes
                             || mesh.sourceVertexCount != sourceVertexCount
                             || mesh.sourceIndexCount != sourceIndexCount
                             || mesh.renderMode != object.renderMode;
    if (needsUpload) {
        GpuMesh uploaded = UploadMeshToGpu(ctx, object);
        if (!uploaded.vertexBuffer || !uploaded.indexBuffer || uploaded.indexCount == 0) {
            return nullptr;
        }
        mesh = std::move(uploaded);
    }

    return &mesh;
}

void GpuResourceCache::Reset() {
    m_meshes.clear();
}

GpuMesh GpuResourceCache::UploadMeshToGpu(RenderContext& ctx, const RenderObject& object) {
    const MeshAsset* const assetMesh = object.mesh;
    const LegacyMeshData3D* const legacyMesh = object.legacyMesh;
    const bool hasAssetMesh = assetMesh != nullptr && !assetMesh->vertices.empty() && !assetMesh->indices.empty();
    const bool hasLegacyMesh = legacyMesh != nullptr && !legacyMesh->vertices.empty() && !legacyMesh->indices.empty();
    if (!hasAssetMesh && !hasLegacyMesh) {
        return {};
    }

    const auto& sourceVertices = hasAssetMesh ? assetMesh->vertices : legacyMesh->vertices;
    const auto& sourceIndices = hasAssetMesh ? assetMesh->indices : legacyMesh->indices;

    GpuMesh mesh{};
    wgpu::BufferDescriptor vertexBufferDesc{};
    vertexBufferDesc.size = sourceVertices.size() * sizeof(Vertex3D);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    mesh.vertexBuffer = ctx.GetDevice()->createBuffer(vertexBufferDesc);
    ctx.GetQueue()->writeBuffer(
        *mesh.vertexBuffer,
        0,
        sourceVertices.data(),
        vertexBufferDesc.size);
    mesh.vertexBufferSize = vertexBufferDesc.size;

    std::vector<uint16_t> indexData;
    std::vector<uint16_t> wireframeDepthIndexData;
    if (object.renderMode == Object3D::ERenderMode::Wireframe) {
        if ((sourceIndices.size() % 3U) != 0U) {
            return {};
        }
        indexData.reserve(sourceIndices.size() * 2U);
        for (std::size_t i = 0; i < sourceIndices.size(); i += 3U) {
            const uint16_t i0 = sourceIndices[i];
            const uint16_t i1 = sourceIndices[i + 1U];
            const uint16_t i2 = sourceIndices[i + 2U];
            indexData.push_back(i0);
            indexData.push_back(i1);
            indexData.push_back(i1);
            indexData.push_back(i2);
            indexData.push_back(i2);
            indexData.push_back(i0);
        }
        wireframeDepthIndexData = sourceIndices;
    } else {
        indexData = sourceIndices;
    }

    wgpu::BufferDescriptor indexBufferDesc{};
    const uint64_t indexBytes = indexData.size() * sizeof(uint16_t);
    indexBufferDesc.size = (indexBytes + 3ull) & ~3ull;
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    mesh.indexBuffer = ctx.GetDevice()->createBuffer(indexBufferDesc);

    std::vector<uint16_t> paddedIndices = indexData;
    if ((paddedIndices.size() & 1u) != 0u) {
        paddedIndices.push_back(0);
    }
    const std::size_t indexWriteSize = std::min<std::size_t>(
        paddedIndices.size() * sizeof(uint16_t),
        indexBufferDesc.size);
    ctx.GetQueue()->writeBuffer(*mesh.indexBuffer, 0, paddedIndices.data(), indexWriteSize);
    mesh.indexBufferSize = indexBufferDesc.size;
    mesh.indexCount = static_cast<uint32_t>(indexData.size());

    if (object.renderMode == Object3D::ERenderMode::Wireframe) {
        wgpu::BufferDescriptor wireframeDepthIndexBufferDesc{};
        const uint64_t wireframeDepthIndexBytes = wireframeDepthIndexData.size() * sizeof(uint16_t);
        wireframeDepthIndexBufferDesc.size = (wireframeDepthIndexBytes + 3ull) & ~3ull;
        wireframeDepthIndexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        wireframeDepthIndexBufferDesc.mappedAtCreation = false;
        mesh.wireframeDepthIndexBuffer = ctx.GetDevice()->createBuffer(wireframeDepthIndexBufferDesc);

        std::vector<uint16_t> paddedWireframeDepthIndices = wireframeDepthIndexData;
        if ((paddedWireframeDepthIndices.size() & 1u) != 0u) {
            paddedWireframeDepthIndices.push_back(0);
        }
        const std::size_t wireframeDepthWriteSize = std::min<std::size_t>(
            paddedWireframeDepthIndices.size() * sizeof(uint16_t),
            wireframeDepthIndexBufferDesc.size);
        ctx.GetQueue()->writeBuffer(
            *mesh.wireframeDepthIndexBuffer,
            0,
            paddedWireframeDepthIndices.data(),
            wireframeDepthWriteSize);
        mesh.wireframeDepthIndexBufferSize = wireframeDepthIndexBufferDesc.size;
        mesh.wireframeDepthIndexCount = static_cast<uint32_t>(wireframeDepthIndexData.size());
    }

    mesh.sourceVertexCount = static_cast<uint32_t>(sourceVertices.size());
    mesh.sourceIndexCount = static_cast<uint32_t>(sourceIndices.size());
    mesh.renderMode = object.renderMode;
    return mesh;
}
