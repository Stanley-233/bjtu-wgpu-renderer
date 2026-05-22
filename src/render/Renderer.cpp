#include "Renderer.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "RenderContext.h"

static ObjectUniformData BuildObjectUniformData(const glm::mat4& worldMatrix) {
    return ObjectUniformData{
        .model = worldMatrix,
        .normalMatrix = glm::transpose(glm::inverse(worldMatrix)),
    };
}

void Renderer::Initialize(RenderContext& renderCtx) {
    m_shadowPass.Initialize(renderCtx);
    m_depthPrepass.Initialize(renderCtx);
    m_sceneNormalPass.Initialize(renderCtx);
    m_ssaoPass.Initialize(renderCtx);
    m_skyboxPass.Initialize(renderCtx, kHdrSceneColorFormat);
    m_forwardOpaquePass.Initialize(renderCtx, kHdrSceneColorFormat);
    m_pbrPass.Initialize(renderCtx, kHdrSceneColorFormat);
    m_toneMapPass.Initialize(renderCtx);
    EnsureFallbackShadowResources(renderCtx);
}

void Renderer::SetSsaoEnabled(const bool enabled) {
    m_ssaoEnabled = enabled;
    m_ssaoPass.SetEnabled(enabled);
}

void Renderer::EnsureFrameResources(RenderContext& renderCtx, const int width, const int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (m_sceneDepthTexture
        && m_sceneAoTexture
        && m_sceneColorTexture
        && m_sceneNormalTexture
        && m_frameResourceWidth == width
        && m_frameResourceHeight == height) {
        return;
    }

    wgpu::TextureDescriptor depthDesc{};
    depthDesc.dimension = wgpu::TextureDimension::_2D;
    depthDesc.size.width = static_cast<uint32_t>(width);
    depthDesc.size.height = static_cast<uint32_t>(height);
    depthDesc.size.depthOrArrayLayers = 1;
    depthDesc.sampleCount = 1;
    depthDesc.mipLevelCount = 1;
    depthDesc.format = wgpu::TextureFormat::Depth24Plus;
    depthDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_sceneDepthTexture = renderCtx.GetDevice()->createTexture(depthDesc);
    m_sceneDepthView = m_sceneDepthTexture->createView();

    wgpu::TextureDescriptor aoDesc{};
    aoDesc.dimension = wgpu::TextureDimension::_2D;
    aoDesc.size.width = static_cast<uint32_t>(width);
    aoDesc.size.height = static_cast<uint32_t>(height);
    aoDesc.size.depthOrArrayLayers = 1;
    aoDesc.sampleCount = 1;
    aoDesc.mipLevelCount = 1;
    aoDesc.format = wgpu::TextureFormat::R8Unorm;
    aoDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_sceneAoTexture = renderCtx.GetDevice()->createTexture(aoDesc);
    m_sceneAoView = m_sceneAoTexture->createView();

    wgpu::TextureDescriptor sceneColorDesc{};
    sceneColorDesc.dimension = wgpu::TextureDimension::_2D;
    sceneColorDesc.size.width = static_cast<uint32_t>(width);
    sceneColorDesc.size.height = static_cast<uint32_t>(height);
    sceneColorDesc.size.depthOrArrayLayers = 1;
    sceneColorDesc.sampleCount = 1;
    sceneColorDesc.mipLevelCount = 1;
    sceneColorDesc.format = kHdrSceneColorFormat;
    sceneColorDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_sceneColorTexture = renderCtx.GetDevice()->createTexture(sceneColorDesc);
    m_sceneColorView = m_sceneColorTexture->createView();

    wgpu::TextureDescriptor sceneNormalDesc{};
    sceneNormalDesc.dimension = wgpu::TextureDimension::_2D;
    sceneNormalDesc.size.width = static_cast<uint32_t>(width);
    sceneNormalDesc.size.height = static_cast<uint32_t>(height);
    sceneNormalDesc.size.depthOrArrayLayers = 1;
    sceneNormalDesc.sampleCount = 1;
    sceneNormalDesc.mipLevelCount = 1;
    sceneNormalDesc.format = wgpu::TextureFormat::RGBA16Float;
    sceneNormalDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_sceneNormalTexture = renderCtx.GetDevice()->createTexture(sceneNormalDesc);
    m_sceneNormalView = m_sceneNormalTexture->createView();

    m_frameResourceWidth = width;
    m_frameResourceHeight = height;
}

void Renderer::EnsureDirectionalShadowResources(RenderContext& renderCtx, const uint32_t width, const uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }
    if (m_directionalShadowTexture
        && m_directionalShadowWidth == width
        && m_directionalShadowHeight == height
        && m_directionalShadowView
        && m_directionalShadowSampler) {
        return;
    }

    wgpu::TextureDescriptor shadowDesc{};
    shadowDesc.dimension = wgpu::TextureDimension::_2D;
    shadowDesc.size.width = width;
    shadowDesc.size.height = height;
    shadowDesc.size.depthOrArrayLayers = 1;
    shadowDesc.sampleCount = 1;
    shadowDesc.mipLevelCount = 1;
    shadowDesc.format = wgpu::TextureFormat::Depth24Plus;
    shadowDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_directionalShadowTexture = renderCtx.GetDevice()->createTexture(shadowDesc);
    m_directionalShadowView = m_directionalShadowTexture->createView();

    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
    samplerDesc.compare = wgpu::CompareFunction::LessEqual;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.maxAnisotropy = 1;
    m_directionalShadowSampler = renderCtx.GetDevice()->createSampler(samplerDesc);

    m_directionalShadowWidth = width;
    m_directionalShadowHeight = height;
}

void Renderer::EnsureFallbackShadowResources(RenderContext& renderCtx) {
    if (m_fallbackShadowTexture && m_fallbackShadowView && m_fallbackShadowSampler) {
        return;
    }

    // TODO: [Shadow] 接入真实的可选 shadow 资源绑定路径后，删除这套 fallback shadow texture/sampler 逻辑。
    wgpu::TextureDescriptor shadowDesc{};
    shadowDesc.dimension = wgpu::TextureDimension::_2D;
    shadowDesc.size.width = 1;
    shadowDesc.size.height = 1;
    shadowDesc.size.depthOrArrayLayers = 1;
    shadowDesc.sampleCount = 1;
    shadowDesc.mipLevelCount = 1;
    shadowDesc.format = wgpu::TextureFormat::Depth24Plus;
    shadowDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_fallbackShadowTexture = renderCtx.GetDevice()->createTexture(shadowDesc);
    m_fallbackShadowView = m_fallbackShadowTexture->createView();

    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
    samplerDesc.compare = wgpu::CompareFunction::LessEqual;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.maxAnisotropy = 1;
    m_fallbackShadowSampler = renderCtx.GetDevice()->createSampler(samplerDesc);
}

RenderFrame Renderer::BeginRenderFrame(RenderContext& renderCtx) {
    RenderFrame frame{};
    frame.clearColor = m_clearColor;
    frame.surfaceFrame = renderCtx.AcquireSurfaceFrame();
    if (!frame.surfaceFrame.view) {
        return frame;
    }

    EnsureFrameResources(
        renderCtx,
        std::max(1, frame.surfaceFrame.surfaceWidth),
        std::max(1, frame.surfaceFrame.surfaceHeight));

    frame.encoder = renderCtx.CreateCommandEncoder();
    if (m_sceneDepthView) {
        frame.sceneDepthView = *m_sceneDepthView;
    }
    if (m_sceneAoView) {
        frame.sceneAoView = *m_sceneAoView;
    }
    if (m_sceneColorView) {
        frame.sceneColorView = *m_sceneColorView;
    }
    if (m_sceneNormalView) {
        frame.sceneNormalView = *m_sceneNormalView;
    }
    return frame;
}

void Renderer::BuildPreparedDrawItems(RenderContext& renderCtx, const RenderScene& scene) {
    m_preparedDrawItems.clear();
    m_drawItemResources.clear();
    m_preparedDrawItems.reserve(scene.objects.size());
    m_drawItemResources.reserve(scene.objects.size());

    for (const RenderObject& object : scene.objects) {
        const GpuMesh* gpuMesh = m_resourceCache.SyncMesh(renderCtx, scene.assetServer, object);
        const GpuResourceCache::GpuMaterialResources* materialResources =
            m_resourceCache.SyncMaterial(renderCtx, scene.assetServer, object);
        if (gpuMesh == nullptr || materialResources == nullptr) {
            continue;
        }

        if (!m_forwardOpaquePass.GetMaterialBindGroupLayout()) {
            continue;
        }

        wgpu::BindGroupEntry materialBindings[5]{};
        materialBindings[0].binding = 0;
        materialBindings[0].buffer = *materialResources->uniformBuffer;
        materialBindings[0].offset = 0;
        materialBindings[0].size = sizeof(MaterialUniformData);
        materialBindings[1].binding = 1;
        materialBindings[1].textureView = materialResources->baseColorTextureView;
        materialBindings[2].binding = 2;
        materialBindings[2].textureView = materialResources->normalTextureView;
        materialBindings[3].binding = 3;
        materialBindings[3].textureView = materialResources->metallicRoughnessTextureView;
        materialBindings[4].binding = 4;
        materialBindings[4].sampler = materialResources->sampler;

        wgpu::BindGroupDescriptor forwardMaterialBindGroupDesc{};
        forwardMaterialBindGroupDesc.layout = *m_forwardOpaquePass.GetMaterialBindGroupLayout();
        forwardMaterialBindGroupDesc.entryCount = 5;
        forwardMaterialBindGroupDesc.entries = materialBindings;

        wgpu::BindGroupDescriptor pbrMaterialBindGroupDesc{};
        pbrMaterialBindGroupDesc.layout = *m_pbrPass.GetMaterialBindGroupLayout();
        pbrMaterialBindGroupDesc.entryCount = 5;
        pbrMaterialBindGroupDesc.entries = materialBindings;
        m_drawItemResources.push_back(DrawItemResources{
            .forwardMaterialBindGroup = renderCtx.GetDevice()->createBindGroup(forwardMaterialBindGroupDesc),
            .pbrMaterialBindGroup = renderCtx.GetDevice()->createBindGroup(pbrMaterialBindGroupDesc),
        });
        const DrawItemResources& resources = m_drawItemResources.back();

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .shadingModel = object.shadingModel,
            .doubleSided = object.doubleSided,
            .model = object.worldMatrix,
            .objectUniformData = BuildObjectUniformData(object.worldMatrix),
            .vertexBuffer = *gpuMesh->vertexBuffer,
            .indexBuffer = *gpuMesh->indexBuffer,
            .forwardMaterialBindGroup = resources.forwardMaterialBindGroup ? *resources.forwardMaterialBindGroup : nullptr,
            .pbrMaterialBindGroup = resources.pbrMaterialBindGroup ? *resources.pbrMaterialBindGroup : nullptr,
            .vertexBufferSize = gpuMesh->vertexBufferSize,
            .indexBufferSize = gpuMesh->indexBufferSize,
            .indexCount = gpuMesh->indexCount,
        });
    }
}

void Renderer::Render(RenderContext& renderCtx, const RenderScene& scene, LegacyGuiRenderer& guiRenderer) {
    RenderFrame frame = BeginRenderFrame(renderCtx);
    if (!frame.surfaceFrame.view || !frame.encoder) {
        return;
    }

    EnsureFallbackShadowResources(renderCtx);
    if (scene.directionalShadow.has_value()) {
        EnsureDirectionalShadowResources(
            renderCtx,
            kDirectionalShadowMapResolution,
            kDirectionalShadowMapResolution);
    }

    BuildPreparedDrawItems(renderCtx, scene);

    std::optional<DirectionalShadowPassData> directionalShadow;
    if (scene.directionalShadow.has_value() && m_directionalShadowView && m_directionalShadowSampler) {
        directionalShadow = DirectionalShadowPassData{
            .uniformData = scene.directionalShadow->uniformData,
            .shadowMapView = *m_directionalShadowView,
            .shadowSampler = *m_directionalShadowSampler,
        };
    }

    const EnvironmentMapGpuResources* skyboxResources = nullptr;
    if (scene.skybox.has_value() && scene.assetServer != nullptr) {
        const HdrImageAsset* hdrImage = scene.assetServer->Get(scene.skybox->hdrImage);
        if (hdrImage != nullptr) {
            skyboxResources = renderCtx.GetEnvironmentMapCache().GetOrCreate(
                renderCtx,
                *hdrImage,
                scene.skybox->faceSize);
        }
    }

    const PassContext passCtx{
        .camera = scene.camera,
        .directionalShadow = directionalShadow,
        .lights = scene.lights,
        .pbrDebugView = scene.pbrDebugView,
        .drawItems = m_preparedDrawItems,
        .guiRenderer = &guiRenderer,
        .queue = &*renderCtx.GetQueue(),
        .skybox = skyboxResources,
        .fallbackShadowMapView = m_fallbackShadowView ? *m_fallbackShadowView : nullptr,
        .fallbackShadowSampler = m_fallbackShadowSampler ? *m_fallbackShadowSampler : nullptr,
        .sceneDepthView = frame.sceneDepthView,
        .sceneAoView = frame.sceneAoView,
        .sceneColorView = frame.sceneColorView,
        .sceneNormalView = frame.sceneNormalView,
        .viewportWidth = frame.surfaceFrame.surfaceWidth,
        .viewportHeight = frame.surfaceFrame.surfaceHeight,
    };
    m_shadowPass.Render(renderCtx, frame, passCtx);
    m_depthPrepass.Render(renderCtx, frame, passCtx);
    m_sceneNormalPass.Render(renderCtx, frame, passCtx);
    m_ssaoPass.Render(renderCtx, frame, passCtx);
    m_skyboxPass.Render(renderCtx, frame, passCtx);
    m_forwardOpaquePass.Render(renderCtx, frame, passCtx);
    m_pbrPass.Render(renderCtx, frame, passCtx);
    m_toneMapPass.Render(renderCtx, frame, passCtx);
    m_guiPass.Render(renderCtx, frame, passCtx);
    renderCtx.Submit(frame.encoder);
    renderCtx.Present(frame.surfaceFrame);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}

void Renderer::PrepareSkybox(RenderContext& renderCtx, const HdrImageAsset& hdrImage, const uint32_t faceSize) {
    (void)renderCtx.GetEnvironmentMapCache().GetOrCreate(renderCtx, hdrImage, faceSize);
}
