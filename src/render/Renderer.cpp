#include "Renderer.h"

#include <algorithm>

#include "GuiRenderer.h"
#include "RenderContext.h"

namespace {
constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;
}

void Renderer::Initialize(RenderContext& ctx) {
    m_forwardPass.Initialize(ctx);
    m_wireframePass.Initialize(ctx);
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
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    ctx.GetDrawableSize(surfaceWidth, surfaceHeight);
    EnsureDepthResources(ctx, std::max(1, surfaceWidth), std::max(1, surfaceHeight));

    RenderFrame frame{};
    frame.drawableWidth = surfaceWidth;
    frame.drawableHeight = surfaceHeight;
    frame.clearColor = m_clearColor;
    frame.surfaceView = ctx.AcquireNextSurfaceView();
    if (!frame.surfaceView) {
        return frame;
    }
    frame.encoder = ctx.BeginFrame();
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
        if (!resources.bindGroup && resources.uniformBuffer && m_forwardPass.GetBindGroupLayout()) {
            wgpu::BindGroupEntry binding{};
            binding.binding = 0;
            binding.buffer = *resources.uniformBuffer;
            binding.offset = 0;
            binding.size = kSceneUniformSize;

            wgpu::BindGroupDescriptor bindGroupDesc{};
            bindGroupDesc.layout = *m_forwardPass.GetBindGroupLayout();
            bindGroupDesc.entryCount = 1;
            bindGroupDesc.entries = &binding;
            resources.bindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);
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
        if (gpuMesh == nullptr) {
            continue;
        }

        ObjectResources& resources = m_objectResources[i];
        if (!resources.uniformBuffer || !resources.bindGroup) {
            continue;
        }

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .model = object.worldMatrix,
            .renderMode = object.renderMode,
            .vertexBuffer = *gpuMesh->vertexBuffer,
            .indexBuffer = *gpuMesh->indexBuffer,
            .wireframeDepthIndexBuffer = gpuMesh->wireframeDepthIndexBuffer ? *gpuMesh->wireframeDepthIndexBuffer : nullptr,
            .uniformBuffer = *resources.uniformBuffer,
            .bindGroup = *resources.bindGroup,
            .vertexBufferSize = gpuMesh->vertexBufferSize,
            .indexBufferSize = gpuMesh->indexBufferSize,
            .wireframeDepthIndexBufferSize = gpuMesh->wireframeDepthIndexBufferSize,
            .indexCount = gpuMesh->indexCount,
            .wireframeDepthIndexCount = gpuMesh->wireframeDepthIndexCount,
        });
    }
}

void Renderer::Render(RenderContext& ctx, const RenderScene& scene, GuiRenderer& guiRenderer) {
    RenderFrame frame = BeginRenderFrame(ctx);
    if (!frame.surfaceView || !frame.encoder) {
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
    m_wireframePass.Render(frame, passContext);
    m_guiPass.Render(frame, passContext);
    ctx.SubmitAndPresent(frame.encoder);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
