#include "GpuResourceCache.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include <glm/vec2.hpp>

#include "asset/types/AssetVertex3D.h"
#include "render/RenderContext.h"

std::size_t GpuResourceCache::CacheKeyHash::operator()(const CacheKey& key) const {
    const auto meshBits = key.meshId.IsValid()
                              ? static_cast<std::uintptr_t>(key.meshId.value)
                              : reinterpret_cast<std::uintptr_t>(key.legacyMesh);
    return meshBits;
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
    };
    auto [it, inserted] = m_meshes.try_emplace(key);
    GpuMesh& mesh = it->second;

    const std::size_t sourceVertexCount = hasAssetMesh ? assetMesh->vertices.size() : legacyMesh->vertices.size();
    const auto& sourceIndices = hasAssetMesh ? assetMesh->indices : legacyMesh->indices;
    const uint64_t vertexBytes = sourceVertexCount * sizeof(AssetVertex3D);
    const uint32_t sourceIndexCount = static_cast<uint32_t>(sourceIndices.size());
    const bool needsUpload = inserted
                             || !mesh.vertexBuffer
                             || !mesh.indexBuffer
                             || mesh.vertexBufferSize != vertexBytes
                             || mesh.sourceVertexCount != static_cast<uint32_t>(sourceVertexCount)
                             || mesh.sourceIndexCount != sourceIndexCount;
    if (needsUpload) {
        GpuMesh uploaded = UploadMeshToGpu(ctx, object);
        if (!uploaded.vertexBuffer || !uploaded.indexBuffer || uploaded.indexCount == 0) {
            return nullptr;
        }
        mesh = std::move(uploaded);
    }

    return &mesh;
}

const GpuResourceCache::GpuMaterialResources* GpuResourceCache::SyncMaterial(
    RenderContext& ctx,
    const AssetServer* assetServer,
    const RenderObject& object) {
    if (!m_sampler) {
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
        samplerDesc.maxAnisotropy = 1;
        m_sampler = ctx.GetDevice()->createSampler(samplerDesc);
    }

    if (object.material == nullptr || !object.materialId.IsValid()) {
        const GpuTexture2D* whiteTexture = GetOrCreateWhiteTexture(ctx);
        if (whiteTexture == nullptr) {
            return nullptr;
        }
        if (!m_defaultMaterial.has_value() || !m_defaultMaterial->uniformBuffer) {
            m_defaultMaterial = BuildMaterialResources(ctx, MaterialAsset{}, *whiteTexture);
        }
        return &*m_defaultMaterial;
    }

    const GpuTexture2D* baseColorTexture = nullptr;
    if (object.material->baseColorTexture.IsValid()) {
        baseColorTexture = SyncTexture(ctx, assetServer, object.material->baseColorTexture);
    }
    if (baseColorTexture == nullptr) {
        baseColorTexture = GetOrCreateWhiteTexture(ctx);
    }
    if (baseColorTexture == nullptr) {
        return nullptr;
    }

    auto [it, inserted] = m_materials.try_emplace(object.materialId);
    GpuMaterialResources& resources = it->second;
    const GpuMaterialResources::MaterialUniformData desiredUniform{
        .baseColorFactor = object.material->baseColorFactor,
    };
    const bool uniformChanged = std::memcmp(
        &resources.uniformData,
        &desiredUniform,
        sizeof(GpuMaterialResources::MaterialUniformData)) != 0;
    const bool needsRebuild = inserted || !resources.uniformBuffer || resources.textureView == nullptr || resources.sampler == nullptr || uniformChanged;
    if (needsRebuild) {
        resources = BuildMaterialResources(ctx, *object.material, *baseColorTexture);
    }
    return &resources;
}

void GpuResourceCache::Reset() {
    m_meshes.clear();
    m_textures.clear();
    m_materials.clear();
    m_whiteTexture.reset();
    m_defaultMaterial.reset();
    m_sampler = {};
}

GpuMesh GpuResourceCache::UploadMeshToGpu(RenderContext& ctx, const RenderObject& object) {
    const MeshAsset* const assetMesh = object.mesh;
    const LegacyMeshData3D* const legacyMesh = object.legacyMesh;
    const bool hasAssetMesh = assetMesh != nullptr && !assetMesh->vertices.empty() && !assetMesh->indices.empty();
    const bool hasLegacyMesh = legacyMesh != nullptr && !legacyMesh->vertices.empty() && !legacyMesh->indices.empty();
    if (!hasAssetMesh && !hasLegacyMesh) {
        return {};
    }

    std::vector<AssetVertex3D> vertexData{};
    const std::vector<uint16_t>* sourceIndices = nullptr;
    if (hasAssetMesh) {
        vertexData = assetMesh->vertices;
        sourceIndices = &assetMesh->indices;
    } else {
        vertexData.reserve(legacyMesh->vertices.size());
        for (const Vertex3D& legacyVertex : legacyMesh->vertices) {
            vertexData.push_back(AssetVertex3D{
                .position = legacyVertex.position,
                .normal = glm::vec3{0.0f, 0.0f, 1.0f},
                .uv0 = glm::vec2{0.0f, 0.0f},
                .color = glm::vec4{legacyVertex.color, 1.0f},
            });
        }
        sourceIndices = &legacyMesh->indices;
    }

    GpuMesh mesh{};
    wgpu::BufferDescriptor vertexBufferDesc{};
    vertexBufferDesc.size = vertexData.size() * sizeof(AssetVertex3D);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    mesh.vertexBuffer = ctx.GetDevice()->createBuffer(vertexBufferDesc);
    ctx.GetQueue()->writeBuffer(*mesh.vertexBuffer, 0, vertexData.data(), vertexBufferDesc.size);
    mesh.vertexBufferSize = vertexBufferDesc.size;

    std::vector<uint16_t> indexData = *sourceIndices;

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

    mesh.sourceVertexCount = static_cast<uint32_t>(vertexData.size());
    mesh.sourceIndexCount = static_cast<uint32_t>(sourceIndices->size());
    return mesh;
}

GpuResourceCache::GpuTexture2D GpuResourceCache::UploadTextureToGpu(RenderContext& ctx, const ImageAsset& image) {
    if (image.width == 0 || image.height == 0 || image.pixels.empty()) {
        return {};
    }

    GpuTexture2D gpuTexture{};
    wgpu::TextureDescriptor textureDesc{};
    textureDesc.dimension = wgpu::TextureDimension::_2D;
    textureDesc.size.width = image.width;
    textureDesc.size.height = image.height;
    textureDesc.size.depthOrArrayLayers = 1;
    textureDesc.sampleCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.format = image.format == ImageAssetFormat::Rgba8Unorm
                             ? wgpu::TextureFormat::RGBA8Unorm
                             : wgpu::TextureFormat::RGBA8UnormSrgb;
    textureDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
    gpuTexture.texture = ctx.GetDevice()->createTexture(textureDesc);
    gpuTexture.view = gpuTexture.texture->createView();

    WGPUImageCopyTexture dstView{};
    dstView.texture = *gpuTexture.texture;
    dstView.mipLevel = 0;
    dstView.origin = {0, 0, 0};
    dstView.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout layout{};
    layout.offset = 0;
    layout.bytesPerRow = image.width * 4U;
    layout.rowsPerImage = image.height;

    WGPUExtent3D size{
        .width = image.width,
        .height = image.height,
        .depthOrArrayLayers = 1,
    };
    wgpuQueueWriteTexture(
        *ctx.GetQueue(),
        &dstView,
        image.pixels.data(),
        static_cast<uint32_t>(image.pixels.size()),
        &layout,
        &size);
    return gpuTexture;
}

const GpuResourceCache::GpuTexture2D* GpuResourceCache::SyncTexture(
    RenderContext& ctx,
    const AssetServer* assetServer,
    const AssetId<ImageAsset> imageId) {
    if (assetServer == nullptr || !imageId.IsValid()) {
        return nullptr;
    }

    const ImageAsset* image = assetServer->GetImage(imageId);
    if (image == nullptr || image->width == 0 || image->height == 0 || image->pixels.empty()) {
        return nullptr;
    }

    auto [it, inserted] = m_textures.try_emplace(imageId);
    if (inserted || !it->second.texture || !it->second.view) {
        it->second = UploadTextureToGpu(ctx, *image);
    }
    if (!it->second.texture || !it->second.view) {
        return nullptr;
    }
    return &it->second;
}

const GpuResourceCache::GpuTexture2D* GpuResourceCache::GetOrCreateWhiteTexture(RenderContext& ctx) {
    if (!m_whiteTexture.has_value() || !m_whiteTexture->texture || !m_whiteTexture->view) {
        ImageAsset whiteImage{};
        whiteImage.width = 1;
        whiteImage.height = 1;
        whiteImage.format = ImageAssetFormat::Rgba8Srgb;
        whiteImage.pixels = {255U, 255U, 255U, 255U};
        m_whiteTexture = UploadTextureToGpu(ctx, whiteImage);
    }
    if (!m_whiteTexture->texture || !m_whiteTexture->view) {
        return nullptr;
    }
    return &*m_whiteTexture;
}

GpuResourceCache::GpuMaterialResources GpuResourceCache::BuildMaterialResources(
    RenderContext& ctx,
    const MaterialAsset& material,
    const GpuTexture2D& baseColorTexture) const {
    GpuMaterialResources resources{};
    resources.uniformData = GpuMaterialResources::MaterialUniformData{
        .baseColorFactor = material.baseColorFactor,
    };

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(GpuMaterialResources::MaterialUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    resources.uniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
    ctx.GetQueue()->writeBuffer(
        *resources.uniformBuffer,
        0,
        &resources.uniformData,
        sizeof(GpuMaterialResources::MaterialUniformData));
    resources.textureView = *baseColorTexture.view;
    resources.sampler = *m_sampler;
    return resources;
}
