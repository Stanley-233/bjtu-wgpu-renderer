#include "Renderer.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "GuiRenderer.h"
#include "PipelineLibrary.h"
#include "RenderContext.h"

namespace {
constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;
}

void Renderer::Initialize(RenderContext& ctx) {
    auto pipeline3D              = PipelineLibrary::CreateColor3D(ctx);
    auto wireframe3D             = PipelineLibrary::CreateColor3DWireframe(ctx);
    auto wireframeDepthPrepass3D = PipelineLibrary::CreateColor3DWireframeDepthPrepass(ctx);
    m_bindGroupLayout            = std::move(pipeline3D.bindGroupLayout);
    m_solidLayout                = std::move(pipeline3D.layout);
    m_solidPipeline              = std::move(pipeline3D.pipeline);
    m_wireframeLayout            = std::move(wireframe3D.layout);
    m_wireframePipeline          = std::move(wireframe3D.pipeline);
    m_wireframeDepthPrepassLayout = std::move(wireframeDepthPrepass3D.layout);
    m_wireframeDepthPrepassPipeline = std::move(wireframeDepthPrepass3D.pipeline);
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

Renderer::DrawItem Renderer::UploadMeshToGpu(RenderContext& ctx, const RenderObject& object) {
    if (object.mesh == nullptr || object.mesh->vertices.empty() || object.mesh->indices.empty()) {
        return {};
    }

    DrawItem drawItem{};
    wgpu::BufferDescriptor vertexBufferDesc{};
    vertexBufferDesc.size = object.mesh->vertices.size() * sizeof(Vertex3D);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    drawItem.vertexBuffer = ctx.GetDevice()->createBuffer(vertexBufferDesc);
    ctx.GetQueue()->writeBuffer(
        *drawItem.vertexBuffer,
        0,
        object.mesh->vertices.data(),
        vertexBufferDesc.size);
    drawItem.vertexBufferSize = vertexBufferDesc.size;

    std::vector<uint16_t> indexData;
    std::vector<uint16_t> wireframeDepthIndexData;
    if (object.renderMode == Object3D::ERenderMode::Wireframe) {
        if ((object.mesh->indices.size() % 3U) != 0U) {
            return {};
        }
        indexData.reserve(object.mesh->indices.size() * 2U);
        for (size_t i = 0; i < object.mesh->indices.size(); i += 3U) {
            const uint16_t i0 = object.mesh->indices[i];
            const uint16_t i1 = object.mesh->indices[i + 1U];
            const uint16_t i2 = object.mesh->indices[i + 2U];
            indexData.push_back(i0);
            indexData.push_back(i1);
            indexData.push_back(i1);
            indexData.push_back(i2);
            indexData.push_back(i2);
            indexData.push_back(i0);
        }
        wireframeDepthIndexData = object.mesh->indices;
    } else {
        indexData = object.mesh->indices;
    }

    wgpu::BufferDescriptor indexBufferDesc{};
    const uint64_t indexBytes = indexData.size() * sizeof(uint16_t);
    indexBufferDesc.size = (indexBytes + 3ull) & ~3ull;
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    drawItem.indexBuffer = ctx.GetDevice()->createBuffer(indexBufferDesc);

    std::vector<uint16_t> paddedIndices = indexData;
    if ((paddedIndices.size() & 1u) != 0u) {
        paddedIndices.push_back(0);
    }
    const std::size_t writeSize = std::min<std::size_t>(
        paddedIndices.size() * sizeof(uint16_t),
        indexBufferDesc.size);
    ctx.GetQueue()->writeBuffer(*drawItem.indexBuffer, 0, paddedIndices.data(), writeSize);
    drawItem.indexBufferSize = indexBufferDesc.size;
    drawItem.indexCount = static_cast<uint32_t>(indexData.size());

    if (object.renderMode == Object3D::ERenderMode::Wireframe) {
        wgpu::BufferDescriptor wireframeDepthIndexBufferDesc{};
        const uint64_t wireframeDepthIndexBytes = wireframeDepthIndexData.size() * sizeof(uint16_t);
        wireframeDepthIndexBufferDesc.size = (wireframeDepthIndexBytes + 3ull) & ~3ull;
        wireframeDepthIndexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        wireframeDepthIndexBufferDesc.mappedAtCreation = false;
        drawItem.wireframeDepthIndexBuffer = ctx.GetDevice()->createBuffer(wireframeDepthIndexBufferDesc);

        std::vector<uint16_t> paddedWireframeDepthIndices = wireframeDepthIndexData;
        if ((paddedWireframeDepthIndices.size() & 1u) != 0u) {
            paddedWireframeDepthIndices.push_back(0);
        }
        const std::size_t wireframeDepthWriteSize = std::min<std::size_t>(
            paddedWireframeDepthIndices.size() * sizeof(uint16_t),
            wireframeDepthIndexBufferDesc.size);
        ctx.GetQueue()->writeBuffer(
            *drawItem.wireframeDepthIndexBuffer,
            0,
            paddedWireframeDepthIndices.data(),
            wireframeDepthWriteSize);
        drawItem.wireframeDepthIndexBufferSize = wireframeDepthIndexBufferDesc.size;
        drawItem.wireframeDepthIndexCount = static_cast<uint32_t>(wireframeDepthIndexData.size());
    }

    drawItem.sourceVertexCount = static_cast<uint32_t>(object.mesh->vertices.size());
    drawItem.sourceIndexCount = static_cast<uint32_t>(object.mesh->indices.size());
    drawItem.renderMode = object.renderMode;
    drawItem.model = object.worldMatrix;
    return drawItem;
}

void Renderer::Render(RenderContext& ctx, const RenderScene& scene, GuiRenderer& guiRenderer) {
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    ctx.GetDrawableSize(surfaceWidth, surfaceHeight);
    EnsureDepthResources(ctx, std::max(1, surfaceWidth), std::max(1, surfaceHeight));

    m_drawItems.resize(scene.objects.size());
    for (std::size_t i = 0; i < scene.objects.size(); ++i) {
        const RenderObject& object = scene.objects[i];
        DrawItem&           drawItem = m_drawItems[i];
        if (object.mesh == nullptr || object.mesh->vertices.empty() || object.mesh->indices.empty()) {
            drawItem = {};
            continue;
        }

        const uint64_t vertexBytes = object.mesh->vertices.size() * sizeof(Vertex3D);
        const uint32_t sourceVertexCount = static_cast<uint32_t>(object.mesh->vertices.size());
        const uint32_t sourceIndexCount = static_cast<uint32_t>(object.mesh->indices.size());
        const bool needUpload = !drawItem.vertexBuffer
                                || !drawItem.indexBuffer
                                || drawItem.vertexBufferSize != vertexBytes
                                || drawItem.sourceVertexCount != sourceVertexCount
                                || drawItem.sourceIndexCount != sourceIndexCount
                                || drawItem.renderMode != object.renderMode;
        if (needUpload) {
            DrawItem uploaded = UploadMeshToGpu(ctx, object);
            if (!uploaded.vertexBuffer || !uploaded.indexBuffer || uploaded.indexCount == 0) {
                drawItem = {};
                continue;
            }
            drawItem.vertexBuffer = std::move(uploaded.vertexBuffer);
            drawItem.indexBuffer = std::move(uploaded.indexBuffer);
            drawItem.wireframeDepthIndexBuffer = std::move(uploaded.wireframeDepthIndexBuffer);
            drawItem.vertexBufferSize = uploaded.vertexBufferSize;
            drawItem.indexBufferSize = uploaded.indexBufferSize;
            drawItem.wireframeDepthIndexBufferSize = uploaded.wireframeDepthIndexBufferSize;
            drawItem.indexCount = uploaded.indexCount;
            drawItem.wireframeDepthIndexCount = uploaded.wireframeDepthIndexCount;
            drawItem.sourceVertexCount = uploaded.sourceVertexCount;
            drawItem.sourceIndexCount = uploaded.sourceIndexCount;
            drawItem.renderMode = uploaded.renderMode;
        }

        if (!drawItem.uniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size = kSceneUniformSize;
            uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            drawItem.uniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
        if (!drawItem.bindGroup) {
            wgpu::BindGroupEntry binding{};
            binding.binding = 0;
            binding.buffer = *drawItem.uniformBuffer;
            binding.offset = 0;
            binding.size = kSceneUniformSize;

            wgpu::BindGroupDescriptor bindGroupDesc{};
            bindGroupDesc.layout = *m_bindGroupLayout;
            bindGroupDesc.entryCount = 1;
            bindGroupDesc.entries = &binding;
            drawItem.bindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);
        }

        if (!drawItem.uniformBuffer || !drawItem.bindGroup) {
            drawItem = {};
            continue;
        }
        drawItem.model = object.worldMatrix;
    }

    wgpu::raii::TextureView targetView = ctx.AcquireNextSurfaceView();
    if (!targetView) {
        return;
    }

    wgpu::raii::CommandEncoder encoder = ctx.BeginFrame();

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = *targetView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = m_clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    if (m_depthView) {
        depthAttachment.view = *m_depthView;
        depthAttachment.depthClearValue = 1.0f;
        depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthReadOnly = false;
        depthAttachment.stencilReadOnly = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }

    wgpu::raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    if (scene.camera.has_value()) {
        for (DrawItem& drawItem : m_drawItems) {
            if (!drawItem.uniformBuffer || !drawItem.bindGroup || !drawItem.vertexBuffer || !drawItem.indexBuffer) {
                continue;
            }

            const SceneUniform uniform{
                .model = drawItem.model,
                .view = scene.camera->view,
                .projection = scene.camera->projection,
            };
            ctx.GetQueue()->writeBuffer(*drawItem.uniformBuffer, 0, &uniform, kSceneUniformSize);
            renderPass->setBindGroup(0, *drawItem.bindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, *drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);

            if (drawItem.renderMode == Object3D::ERenderMode::Wireframe) {
                if (m_wireframeDepthPrepassPipeline
                    && drawItem.wireframeDepthIndexBuffer
                    && drawItem.wireframeDepthIndexCount > 0) {
                    renderPass->setPipeline(*m_wireframeDepthPrepassPipeline);
                    renderPass->setIndexBuffer(
                        *drawItem.wireframeDepthIndexBuffer,
                        wgpu::IndexFormat::Uint16,
                        0,
                        drawItem.wireframeDepthIndexBufferSize);
                    renderPass->drawIndexed(drawItem.wireframeDepthIndexCount, 1, 0, 0, 0);
                }
                if (!m_wireframePipeline) {
                    continue;
                }
                renderPass->setPipeline(*m_wireframePipeline);
            } else {
                if (!m_solidPipeline) {
                    continue;
                }
                renderPass->setPipeline(*m_solidPipeline);
            }

            renderPass->setIndexBuffer(*drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();

    wgpu::RenderPassColorAttachment uiColorAttachment{};
    uiColorAttachment.view = *targetView;
    uiColorAttachment.resolveTarget = nullptr;
    uiColorAttachment.loadOp = wgpu::LoadOp::Load;
    uiColorAttachment.storeOp = wgpu::StoreOp::Store;
    uiColorAttachment.clearValue = m_clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    uiColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor uiPassDesc{};
    uiPassDesc.colorAttachmentCount = 1;
    uiPassDesc.colorAttachments = &uiColorAttachment;
    uiPassDesc.depthStencilAttachment = nullptr;
    uiPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder uiPass = encoder->beginRenderPass(uiPassDesc);
    guiRenderer.Render(uiPass);
    uiPass->end();

    ctx.SubmitAndPresent(encoder);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
