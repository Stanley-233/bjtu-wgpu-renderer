#include "Renderer.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "RenderContext.h"

namespace {
ObjectUniformData BuildObjectUniformData(const glm::mat4& worldMatrix) {
    return ObjectUniformData{
        .model = worldMatrix,
        .normalMatrix = glm::transpose(glm::inverse(worldMatrix)),
    };
}
} // namespace

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

        if (!m_forwardPass.GetMaterialBindGroupLayout()) {
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
        materialBindGroupDesc.layout = *m_forwardPass.GetMaterialBindGroupLayout();
        materialBindGroupDesc.entryCount = 3;
        materialBindGroupDesc.entries = materialBindings;
        m_drawItemResources.push_back(DrawItemResources{
            .materialBindGroup = ctx.GetDevice()->createBindGroup(materialBindGroupDesc),
        });
        const DrawItemResources& resources = m_drawItemResources.back();

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .shadingModel = static_cast<EMaterialShadingModel>(materialResources->uniformData.surfaceOptions.x),
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

    BuildPreparedDrawItems(ctx, scene);

    const PassContext passContext{
        .camera = scene.camera,
        .lights = scene.lights,
        .drawItems = m_preparedDrawItems,
        .guiRenderer = &guiRenderer,
        .queue = &*ctx.GetQueue(),
    };
    m_forwardPass.Render(ctx, frame, passContext);
    m_guiPass.Render(ctx, frame, passContext);
    ctx.Submit(frame.encoder);
    ctx.Present(frame.surfaceFrame);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
