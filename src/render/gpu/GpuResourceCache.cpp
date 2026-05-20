#include "GpuResourceCache.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include <glm/vec2.hpp>

#include "asset/types/AssetVertex3D.h"
#include "render/RenderContext.h"

namespace {
MaterialUniformData BuildMaterialUniformData(
    const MaterialAsset& material,
    const EMaterialShadingModel shadingModel) {
    return MaterialUniformData{
        .baseColorFactor = material.baseColorFactor,
        .surfaceOptions = glm::uvec4{
            static_cast<uint32_t>(shadingModel),
            material.useVertexColor ? 1U : 0U,
            0U,
            0U,
        },
    };
}
} // namespace

std::size_t GpuResourceCache::CacheKeyHash::operator()(const CacheKey& key) const {
    const auto meshBits = key.meshId.IsValid()
                              ? static_cast<std::uintptr_t>(key.meshId.value)
                              : reinterpret_cast<std::uintptr_t>(key.legacyMesh);
    return meshBits;
}

std::size_t GpuResourceCache::MaterialCacheKeyHash::operator()(const MaterialCacheKey& key) const {
    return (static_cast<std::size_t>(key.materialId.value) << 2U)
           ^ static_cast<std::size_t>(key.shadingModel);
}

const GpuMesh* GpuResourceCache::SyncMesh(RenderContext& renderCtx, const AssetServer* assetServer, const RenderObject& object) {
    const MeshAsset* const assetMesh = assetServer != nullptr ? assetServer->Get(object.meshId) : nullptr;
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
        GpuMesh uploaded = UploadMeshToGpu(renderCtx, assetMesh, legacyMesh);
        if (!uploaded.vertexBuffer || !uploaded.indexBuffer || uploaded.indexCount == 0) {
            return nullptr;
        }
        mesh = std::move(uploaded);
    }

    return &mesh;
}

const GpuResourceCache::GpuMaterialResources* GpuResourceCache::SyncMaterial(
    RenderContext& renderCtx,
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
        m_sampler = renderCtx.GetDevice()->createSampler(samplerDesc);
    }

    const MaterialAsset* const material = assetServer != nullptr ? assetServer->Get(object.materialId) : nullptr;
    const MaterialAsset fallbackMaterial{};
    const MaterialAsset& effectiveMaterial = material != nullptr && object.materialId.IsValid()
                                                 ? *material
                                                 : fallbackMaterial;

    const GpuTexture2D* baseColorTexture = nullptr;
    if (effectiveMaterial.baseColorTexture.IsValid()) {
        baseColorTexture = SyncTexture(renderCtx, assetServer, effectiveMaterial.baseColorTexture);
    }
    if (baseColorTexture == nullptr) {
        baseColorTexture = GetOrCreateWhiteTexture(renderCtx);
    }
    if (baseColorTexture == nullptr) {
        return nullptr;
    }

    const MaterialCacheKey key{
        .materialId = object.materialId,
        .shadingModel = object.shadingModel,
    };
    auto [it, inserted] = m_materials.try_emplace(key);
    GpuMaterialResources& resources = it->second;
    const MaterialUniformData desiredUniform = BuildMaterialUniformData(effectiveMaterial, object.shadingModel);
    const bool uniformChanged = std::memcmp(
        &resources.uniformData,
        &desiredUniform,
        sizeof(MaterialUniformData)) != 0;
    const bool textureChanged = resources.textureView == nullptr || resources.textureView != *baseColorTexture->view;
    const bool samplerChanged = resources.sampler == nullptr || resources.sampler != *m_sampler;
    const bool needsRebuild = inserted
                              || !resources.uniformBuffer
                              || textureChanged
                              || samplerChanged
                              || uniformChanged;
    if (needsRebuild) {
        resources = BuildMaterialResources(renderCtx, effectiveMaterial, object.shadingModel, *baseColorTexture);
    }
    return &resources;
}

void GpuResourceCache::Reset() {
    m_meshes.clear();
    m_textures.clear();
    m_materials.clear();
    m_whiteTexture.reset();
    m_sampler = {};
}

GpuMesh GpuResourceCache::UploadMeshToGpu(RenderContext& renderCtx, const MeshAsset* assetMesh, const LegacyMeshData3D* legacyMesh) {
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
    mesh.vertexBuffer = renderCtx.GetDevice()->createBuffer(vertexBufferDesc);
    renderCtx.GetQueue()->writeBuffer(*mesh.vertexBuffer, 0, vertexData.data(), vertexBufferDesc.size);
    mesh.vertexBufferSize = vertexBufferDesc.size;

    std::vector<uint16_t> indexData = *sourceIndices;

    wgpu::BufferDescriptor indexBufferDesc{};
    const uint64_t indexBytes = indexData.size() * sizeof(uint16_t);
    indexBufferDesc.size = (indexBytes + 3ull) & ~3ull;
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    mesh.indexBuffer = renderCtx.GetDevice()->createBuffer(indexBufferDesc);

    std::vector<uint16_t> paddedIndices = indexData;
    if ((paddedIndices.size() & 1u) != 0u) {
        paddedIndices.push_back(0);
    }
    const std::size_t indexWriteSize = std::min<std::size_t>(
        paddedIndices.size() * sizeof(uint16_t),
        indexBufferDesc.size);
    renderCtx.GetQueue()->writeBuffer(*mesh.indexBuffer, 0, paddedIndices.data(), indexWriteSize);
    mesh.indexBufferSize = indexBufferDesc.size;
    mesh.indexCount = static_cast<uint32_t>(indexData.size());

    mesh.sourceVertexCount = static_cast<uint32_t>(vertexData.size());
    mesh.sourceIndexCount = static_cast<uint32_t>(sourceIndices->size());
    return mesh;
}

GpuResourceCache::GpuTexture2D GpuResourceCache::UploadTextureToGpu(RenderContext& renderCtx, const ImageAsset& image) {
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
    textureDesc.format = image.format == EImageAssetFormat::Rgba8Unorm
                             ? wgpu::TextureFormat::RGBA8Unorm
                             : wgpu::TextureFormat::RGBA8UnormSrgb;
    textureDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
    gpuTexture.texture = renderCtx.GetDevice()->createTexture(textureDesc);
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
        *renderCtx.GetQueue(),
        &dstView,
        image.pixels.data(),
        static_cast<uint32_t>(image.pixels.size()),
        &layout,
        &size);
    return gpuTexture;
}

const GpuResourceCache::GpuTexture2D* GpuResourceCache::SyncTexture(
    RenderContext& renderCtx,
    const AssetServer* assetServer,
    const AssetId<ImageAsset> imageId) {
    if (assetServer == nullptr || !imageId.IsValid()) {
        return nullptr;
    }

    const ImageAsset* image = assetServer->Get(imageId);
    if (image == nullptr || image->width == 0 || image->height == 0 || image->pixels.empty()) {
        return nullptr;
    }

    auto [it, inserted] = m_textures.try_emplace(imageId);
    if (inserted || !it->second.texture || !it->second.view) {
        it->second = UploadTextureToGpu(renderCtx, *image);
    }
    if (!it->second.texture || !it->second.view) {
        return nullptr;
    }
    return &it->second;
}

const GpuResourceCache::GpuTexture2D* GpuResourceCache::GetOrCreateWhiteTexture(RenderContext& renderCtx) {
    if (!m_whiteTexture.has_value() || !m_whiteTexture->texture || !m_whiteTexture->view) {
        ImageAsset whiteImage{};
        whiteImage.width = 1;
        whiteImage.height = 1;
        whiteImage.format = EImageAssetFormat::Rgba8Srgb;
        whiteImage.pixels = {255U, 255U, 255U, 255U};
        m_whiteTexture = UploadTextureToGpu(renderCtx, whiteImage);
    }
    if (!m_whiteTexture->texture || !m_whiteTexture->view) {
        return nullptr;
    }
    return &*m_whiteTexture;
}

GpuResourceCache::GpuMaterialResources GpuResourceCache::BuildMaterialResources(
    RenderContext& renderCtx,
    const MaterialAsset& material,
    const EMaterialShadingModel shadingModel,
    const GpuTexture2D& baseColorTexture) const {
    GpuMaterialResources resources{};
    resources.uniformData = BuildMaterialUniformData(material, shadingModel);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(MaterialUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    resources.uniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
    renderCtx.GetQueue()->writeBuffer(
        *resources.uniformBuffer,
        0,
        &resources.uniformData,
        sizeof(MaterialUniformData));
    resources.textureView = *baseColorTexture.view;
    resources.sampler = *m_sampler;
    return resources;
}
