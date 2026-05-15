#include "Renderer.h"

#include <algorithm>

#include "RenderContext.h"

constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;

void Renderer::Initialize(RenderContext& ctx) {
    m_forwardPass.Initialize(ctx);
}

void Renderer::EnsureDepthResources(RenderContext& ctx, const int width, const int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (m_depthTexture && m_depthWidth == width && m_depthHeight == height) {
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
    depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
    m_depthTexture = ctx.GetDevice()->createTexture(depthDesc);
    m_depthView = m_depthTexture->createView();
    m_depthWidth = width;
    m_depthHeight = height;
}

RenderFrame Renderer::BeginRenderFrame(RenderContext& ctx) {
    RenderFrame frame{};
    frame.clearColor = m_clearColor;
    frame.surfaceFrame = ctx.AcquireSurfaceFrame();
    if (!frame.surfaceFrame.view) {
        return frame;
    }

    EnsureDepthResources(
        ctx,
        std::max(1, frame.surfaceFrame.surfaceWidth),
        std::max(1, frame.surfaceFrame.surfaceHeight));

    frame.encoder = ctx.CreateCommandEncoder();
    if (m_depthView) {
        frame.depthView = *m_depthView;
    }
    return frame;
}

void Renderer::EnsureObjectResources(RenderContext& ctx, const std::size_t objectCount) {
    if (m_objectResources.size() < objectCount) {
        m_objectResources.resize(objectCount);
    }

    for (std::size_t i = 0; i < objectCount; ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.uniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size = kSceneUniformSize;
            uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            resources.uniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
    }
}

void Renderer::BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene) {
    EnsureObjectResources(ctx, scene.objects.size());

    m_preparedDrawItems.clear();
    m_preparedDrawItems.reserve(scene.objects.size());

    for (std::size_t i = 0; i < scene.objects.size(); ++i) {
        const RenderObject& object = scene.objects[i];
        const GpuMesh* gpuMesh = m_resourceCache.SyncMesh(ctx, object);
        const GpuResourceCache::GpuMaterialResources* materialResources =
            m_resourceCache.SyncMaterial(ctx, scene.assetServer, object);
        if (gpuMesh == nullptr || materialResources == nullptr) {
            continue;
        }

        ObjectResources& resources = m_objectResources[i];
        if (!resources.uniformBuffer || !m_forwardPass.GetBindGroupLayout()) {
            continue;
        }
        wgpu::BindGroupEntry bindings[4]{};
        bindings[0].binding = 0;
        bindings[0].buffer = *resources.uniformBuffer;
        bindings[0].offset = 0;
        bindings[0].size = kSceneUniformSize;
        bindings[1].binding = 1;
        bindings[1].buffer = *materialResources->uniformBuffer;
        bindings[1].offset = 0;
        bindings[1].size = sizeof(GpuResourceCache::GpuMaterialResources::MaterialUniformData);
        bindings[2].binding = 2;
        bindings[2].textureView = materialResources->textureView;
        bindings[3].binding = 3;
        bindings[3].sampler = materialResources->sampler;

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = *m_forwardPass.GetBindGroupLayout();
        bindGroupDesc.entryCount = 4;
        bindGroupDesc.entries = bindings;
        resources.forwardBindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .model = object.worldMatrix,
            .vertexBuffer = *gpuMesh->vertexBuffer,
            .indexBuffer = *gpuMesh->indexBuffer,
            .uniformBuffer = *resources.uniformBuffer,
            .forwardBindGroup = resources.forwardBindGroup ? *resources.forwardBindGroup : nullptr,
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

    BuildPreparedDrawItems(ctx, scene);

    const PassContext passContext{
        .camera = scene.camera,
        .drawItems = m_preparedDrawItems,
        .guiRenderer = &guiRenderer,
        .queue = &*ctx.GetQueue(),
    };
    m_forwardPass.Render(frame, passContext);
    m_guiPass.Render(frame, passContext);
    ctx.Submit(frame.encoder);
    ctx.Present(frame.surfaceFrame);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
