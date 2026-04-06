#include "Renderer3D.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <GLFW/glfw3.h>

#include "PipelineLibrary.h"
#include "RenderContext.h"
#include "../resource/models/MeshData3D.h"
#include "../scene/camera/Camera.h"
#include "../scene/scene3d/Object3D.h"

constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;

void Renderer3D::Initialize(RenderContext& ctx) {
    auto pipeline3D   = PipelineLibrary::CreateColor3D(ctx);
    m_bindGroupLayout = std::move(pipeline3D.bindGroupLayout);
    m_layout          = std::move(pipeline3D.layout);
    m_pipeline        = std::move(pipeline3D.pipeline);
}

void Renderer3D::EnsureUniformResources(RenderContext& ctx) {
    if (!m_uniformBuffer) {
        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.size             = kSceneUniformSize;
        bufferDesc.usage            = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        bufferDesc.mappedAtCreation = false;
        m_uniformBuffer             = ctx.GetDevice()->createBuffer(bufferDesc);
    }

    if (!m_bindGroup) {
        wgpu::BindGroupEntry binding{};
        binding.binding = 0;
        binding.buffer  = *m_uniformBuffer;
        binding.offset  = 0;
        binding.size    = kSceneUniformSize;

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout     = *m_bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries    = &binding;
        m_bindGroup              = ctx.GetDevice()->createBindGroup(bindGroupDesc);
    }
}

void Renderer3D::EnsureDepthResources(RenderContext& ctx, const int width, const int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    if (m_depthTexture && m_depthWidth == width && m_depthHeight == height) {
        return;
    }

    wgpu::TextureDescriptor depthDesc{};
    depthDesc.dimension          = wgpu::TextureDimension::_2D;
    depthDesc.size.width         = static_cast<uint32_t>(width);
    depthDesc.size.height        = static_cast<uint32_t>(height);
    depthDesc.size.depthOrArrayLayers = 1;
    depthDesc.sampleCount        = 1;
    depthDesc.mipLevelCount      = 1;
    depthDesc.format             = wgpu::TextureFormat::Depth24Plus;
    depthDesc.usage              = wgpu::TextureUsage::RenderAttachment;
    m_depthTexture               = ctx.GetDevice()->createTexture(depthDesc);
    m_depthView                  = m_depthTexture->createView();
    m_depthWidth                 = width;
    m_depthHeight                = height;
}

Renderer3D::DrawItem Renderer3D::UploadMeshToGpu(RenderContext& ctx, const Object3D& object) {
    const MeshData3D& mesh = object.Mesh();
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return {};
    }

    DrawItem drawItem{};
    wgpu::BufferDescriptor vertexBufferDesc{};
    vertexBufferDesc.size             = mesh.vertices.size() * sizeof(Vertex3D);
    vertexBufferDesc.usage            = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    drawItem.vertexBuffer             = ctx.GetDevice()->createBuffer(vertexBufferDesc);
    ctx.GetQueue()->writeBuffer(
        *drawItem.vertexBuffer,
        0,
        mesh.vertices.data(),
        vertexBufferDesc.size);
    drawItem.vertexBufferSize = vertexBufferDesc.size;

    wgpu::BufferDescriptor indexBufferDesc{};
    // 原始索引数据字节数（每个索引是 uint16_t，占 2 字节）
    const uint64_t indexBytes = mesh.indices.size() * sizeof(uint16_t);
    // WebGPU 要求拷贝/缓冲区大小按 4 字节对齐：把 indexBytes 向上取整到最近的 4 的倍数
    const uint64_t alignedIndexBytes = (indexBytes + 3ull) & ~3ull;
    indexBufferDesc.size             = alignedIndexBytes;
    indexBufferDesc.usage            = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    drawItem.indexBuffer             = ctx.GetDevice()->createBuffer(indexBufferDesc);

    // WebGPU 要求 writeBuffer 的拷贝字节数满足 COPY_BUFFER_ALIGNMENT(4字节对齐)
    // 当 uint16_t 索引个数为奇数时，原始字节数会是 2 的倍数但不是 4 的倍数，此处补一个索引位做 padding
    std::vector<uint16_t> paddedIndices = mesh.indices;
    if ((paddedIndices.size() & 1u) != 0u) {
        paddedIndices.push_back(0);
    }
    const std::size_t writeSize = std::min<std::size_t>(
        paddedIndices.size() * sizeof(uint16_t),
        indexBufferDesc.size);
    ctx.GetQueue()->writeBuffer(*drawItem.indexBuffer, 0, paddedIndices.data(), writeSize);
    drawItem.indexBufferSize = indexBufferDesc.size;
    drawItem.indexCount      = static_cast<uint32_t>(mesh.indices.size());
    drawItem.model           = object.Transform().Matrix();

    return drawItem;
}

void Renderer3D::SyncScene(RenderContext& ctx, const std::vector<Object3D>& objects, const Camera& camera) {
    EnsureUniformResources(ctx);

    int surfaceWidth  = 0;
    int surfaceHeight = 0;
    glfwGetWindowSize(ctx.GetWindow(), &surfaceWidth, &surfaceHeight);
    if (surfaceHeight <= 0) {
        surfaceHeight = 1;
    }
    const float aspect = static_cast<float>(surfaceWidth) / static_cast<float>(surfaceHeight);

    m_view       = camera.View();
    m_projection = camera.Projection(aspect);

    EnsureDepthResources(ctx, surfaceWidth, surfaceHeight);

    m_drawItems.clear();
    m_drawItems.reserve(objects.size());
    for (const Object3D& object : objects) {
        const MeshData3D& mesh = object.Mesh();
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            continue;
        }

        DrawItem drawItem = UploadMeshToGpu(ctx, object);
        if (!drawItem.vertexBuffer || !drawItem.indexBuffer || drawItem.indexCount == 0) {
            continue;
        }

        m_drawItems.push_back(std::move(drawItem));
    }
}

void Renderer3D::RenderFrame(RenderContext& ctx) {
    wgpu::raii::TextureView targetView = ctx.AcquireNextSurfaceView();
    if (!targetView) {
        return;
    }

    wgpu::raii::CommandEncoder encoder = ctx.BeginFrame();

    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view                            = *targetView;
    colorAttachment.resolveTarget                   = nullptr;
    colorAttachment.loadOp                          = wgpu::LoadOp::Clear;
    colorAttachment.storeOp                         = wgpu::StoreOp::Store;
    colorAttachment.clearValue                      = m_clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount       = 1;
    renderPassDesc.colorAttachments           = &colorAttachment;
    renderPassDesc.timestampWrites            = nullptr;

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    if (m_depthView) {
        depthAttachment.view              = *m_depthView;
        depthAttachment.depthClearValue   = 1.0f;
        depthAttachment.depthLoadOp       = wgpu::LoadOp::Clear;
        depthAttachment.depthStoreOp      = wgpu::StoreOp::Store;
        depthAttachment.depthReadOnly     = false;
        depthAttachment.stencilClearValue = 0;
        depthAttachment.stencilLoadOp     = wgpu::LoadOp::Clear;
        depthAttachment.stencilStoreOp    = wgpu::StoreOp::Store;
        depthAttachment.stencilReadOnly   = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }

    wgpu::raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    renderPass->setPipeline(*m_pipeline);
    renderPass->setBindGroup(0, *m_bindGroup, 0, nullptr);

    for (const DrawItem& drawItem : m_drawItems) {
        if (!drawItem.vertexBuffer || !drawItem.indexBuffer || drawItem.indexCount == 0) {
            continue;
        }

        const SceneUniform uniform{
            .model      = drawItem.model,
            .view       = m_view,
            .projection = m_projection,
        };
        ctx.GetQueue()->writeBuffer(*m_uniformBuffer, 0, &uniform, kSceneUniformSize);

        renderPass->setVertexBuffer(0, *drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
        renderPass->setIndexBuffer(*drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
        renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
    }
    renderPass->end();

    ctx.SubmitAndPresent(encoder);
}

void Renderer3D::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}

void Renderer3D::ResetGpuResources() {
    m_uniformBuffer   = {};
    m_layout          = {};
    m_bindGroupLayout = {};
    m_bindGroup       = {};
    m_pipeline        = {};
    m_depthTexture    = {};
    m_depthView       = {};
    m_depthWidth      = 0;
    m_depthHeight     = 0;
    m_view            = glm::mat4(1.0f);
    m_projection      = glm::mat4(1.0f);
    m_drawItems.clear();
}
