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

void Renderer::Initialize(RenderContext& ctx) {
    m_shadowPass.Initialize(ctx);
    m_depthPrepass.Initialize(ctx);
    m_sceneNormalPass.Initialize(ctx);
    m_ssaoPass.Initialize(ctx);
    m_forwardOpaquePass.Initialize(ctx);
    m_compositePass.Initialize(ctx);
    EnsureFallbackShadowResources(ctx);
}

void Renderer::EnsureFrameResources(RenderContext& ctx, const int width, const int height) {
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
    m_sceneDepthTexture = ctx.GetDevice()->createTexture(depthDesc);
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
    m_sceneAoTexture = ctx.GetDevice()->createTexture(aoDesc);
    m_sceneAoView = m_sceneAoTexture->createView();

    wgpu::TextureDescriptor sceneColorDesc{};
    sceneColorDesc.dimension = wgpu::TextureDimension::_2D;
    sceneColorDesc.size.width = static_cast<uint32_t>(width);
    sceneColorDesc.size.height = static_cast<uint32_t>(height);
    sceneColorDesc.size.depthOrArrayLayers = 1;
    sceneColorDesc.sampleCount = 1;
    sceneColorDesc.mipLevelCount = 1;
    sceneColorDesc.format = ctx.GetSurfaceFormat();
    sceneColorDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m_sceneColorTexture = ctx.GetDevice()->createTexture(sceneColorDesc);
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
    m_sceneNormalTexture = ctx.GetDevice()->createTexture(sceneNormalDesc);
    m_sceneNormalView = m_sceneNormalTexture->createView();

    m_frameResourceWidth = width;
    m_frameResourceHeight = height;
}

void Renderer::EnsureDirectionalShadowResources(RenderContext& ctx, const uint32_t width, const uint32_t height) {
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
    m_directionalShadowTexture = ctx.GetDevice()->createTexture(shadowDesc);
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
    m_directionalShadowSampler = ctx.GetDevice()->createSampler(samplerDesc);

    m_directionalShadowWidth = width;
    m_directionalShadowHeight = height;
}

void Renderer::EnsureFallbackShadowResources(RenderContext& ctx) {
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
    m_fallbackShadowTexture = ctx.GetDevice()->createTexture(shadowDesc);
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
    m_fallbackShadowSampler = ctx.GetDevice()->createSampler(samplerDesc);
}

RenderFrame Renderer::BeginRenderFrame(RenderContext& ctx) {
    RenderFrame frame{};
    frame.clearColor = m_clearColor;
    frame.surfaceFrame = ctx.AcquireSurfaceFrame();
    if (!frame.surfaceFrame.view) {
        return frame;
    }

    EnsureFrameResources(
        ctx,
        std::max(1, frame.surfaceFrame.surfaceWidth),
        std::max(1, frame.surfaceFrame.surfaceHeight));

    frame.encoder = ctx.CreateCommandEncoder();
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

void Renderer::BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene) {
    m_preparedDrawItems.clear();
    m_drawItemResources.clear();
    m_preparedDrawItems.reserve(scene.objects.size());
    m_drawItemResources.reserve(scene.objects.size());

    for (const RenderObject& object : scene.objects) {
        const GpuMesh* gpuMesh = m_resourceCache.SyncMesh(ctx, scene.assetServer, object);
        const GpuResourceCache::GpuMaterialResources* materialResources =
            m_resourceCache.SyncMaterial(ctx, scene.assetServer, object);
        if (gpuMesh == nullptr || materialResources == nullptr) {
            continue;
        }

        if (!m_forwardOpaquePass.GetMaterialBindGroupLayout()) {
            continue;
        }

        wgpu::BindGroupEntry materialBindings[3]{};
        materialBindings[0].binding = 0;
        materialBindings[0].buffer = *materialResources->uniformBuffer;
        materialBindings[0].offset = 0;
        materialBindings[0].size = sizeof(MaterialUniformData);
        materialBindings[1].binding = 1;
        materialBindings[1].textureView = materialResources->textureView;
        materialBindings[2].binding = 2;
        materialBindings[2].sampler = materialResources->sampler;

        wgpu::BindGroupDescriptor materialBindGroupDesc{};
        materialBindGroupDesc.layout = *m_forwardOpaquePass.GetMaterialBindGroupLayout();
        materialBindGroupDesc.entryCount = 3;
        materialBindGroupDesc.entries = materialBindings;
        m_drawItemResources.push_back(DrawItemResources{
            .materialBindGroup = ctx.GetDevice()->createBindGroup(materialBindGroupDesc),
        });
        const DrawItemResources& resources = m_drawItemResources.back();

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .shadingModel = object.shadingModel,
            .model = object.worldMatrix,
            .objectUniformData = BuildObjectUniformData(object.worldMatrix),
            .vertexBuffer = *gpuMesh->vertexBuffer,
            .indexBuffer = *gpuMesh->indexBuffer,
            .materialBindGroup = resources.materialBindGroup ? *resources.materialBindGroup : nullptr,
            .vertexBufferSize = gpuMesh->vertexBufferSize,
            .indexBufferSize = gpuMesh->indexBufferSize,
            .indexCount = gpuMesh->indexCount,
        });
    }
}

void Renderer::Render(RenderContext& ctx, const RenderScene& scene, LegacyGuiRenderer& guiRenderer) {
    RenderFrame frame = BeginRenderFrame(ctx);
    if (!frame.surfaceFrame.view || !frame.encoder) {
        return;
    }

    EnsureFallbackShadowResources(ctx);
    if (scene.directionalShadow.has_value()) {
        EnsureDirectionalShadowResources(
            ctx,
            kDirectionalShadowMapResolution,
            kDirectionalShadowMapResolution);
    }

    BuildPreparedDrawItems(ctx, scene);

    std::optional<DirectionalShadowPassData> directionalShadow;
    if (scene.directionalShadow.has_value() && m_directionalShadowView && m_directionalShadowSampler) {
        directionalShadow = DirectionalShadowPassData{
            .uniformData = scene.directionalShadow->uniformData,
            .shadowMapView = *m_directionalShadowView,
            .shadowSampler = *m_directionalShadowSampler,
        };
    }

    const PassContext passContext{
        .camera = scene.camera,
        .directionalShadow = directionalShadow,
        .lights = scene.lights,
        .drawItems = m_preparedDrawItems,
        .guiRenderer = &guiRenderer,
        .queue = &*ctx.GetQueue(),
        .fallbackShadowMapView = m_fallbackShadowView ? *m_fallbackShadowView : nullptr,
        .fallbackShadowSampler = m_fallbackShadowSampler ? *m_fallbackShadowSampler : nullptr,
        .sceneDepthView = frame.sceneDepthView,
        .sceneAoView = frame.sceneAoView,
        .sceneColorView = frame.sceneColorView,
        .sceneNormalView = frame.sceneNormalView,
        .viewportWidth = frame.surfaceFrame.surfaceWidth,
        .viewportHeight = frame.surfaceFrame.surfaceHeight,
    };
    m_shadowPass.Render(ctx, frame, passContext);
    m_depthPrepass.Render(ctx, frame, passContext);
    m_sceneNormalPass.Render(ctx, frame, passContext);
    m_ssaoPass.Render(ctx, frame, passContext);
    m_forwardOpaquePass.Render(ctx, frame, passContext);
    m_compositePass.Render(ctx, frame, passContext);
    m_guiPass.Render(ctx, frame, passContext);
    ctx.Submit(frame.encoder);
    ctx.Present(frame.surfaceFrame);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
