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
    auto wireframe3D  = PipelineLibrary::CreateColor3DWireframe(ctx);
    m_bindGroupLayout = std::move(pipeline3D.bindGroupLayout);
    m_solidLayout     = std::move(pipeline3D.layout);
    m_solidPipeline   = std::move(pipeline3D.pipeline);
    m_wireframeLayout = std::move(wireframe3D.layout);
    m_wireframePipeline = std::move(wireframe3D.pipeline);
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
    const Object3D::ERenderMode renderMode = object.RenderMode();

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

    std::vector<uint16_t> indexData;
    if (renderMode == Object3D::ERenderMode::Wireframe) {
        if ((mesh.indices.size() % 3U) != 0U) {
            return {};
        }
        indexData.reserve(mesh.indices.size() * 2U);
        for (size_t i = 0; i < mesh.indices.size(); i += 3U) {
            const uint16_t i0 = mesh.indices[i];
            const uint16_t i1 = mesh.indices[i + 1U];
            const uint16_t i2 = mesh.indices[i + 2U];
            indexData.push_back(i0);
            indexData.push_back(i1);
            indexData.push_back(i1);
            indexData.push_back(i2);
            indexData.push_back(i2);
            indexData.push_back(i0);
        }
    } else {
        indexData = mesh.indices;
    }

    wgpu::BufferDescriptor indexBufferDesc{};
    const uint64_t indexBytes = indexData.size() * sizeof(uint16_t);
    // WebGPU 要求拷贝/缓冲区大小按 4 字节对齐：把 indexBytes 向上取整到最近的 4 的倍数
    const uint64_t alignedIndexBytes = (indexBytes + 3ull) & ~3ull;
    indexBufferDesc.size             = alignedIndexBytes;
    indexBufferDesc.usage            = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    drawItem.indexBuffer             = ctx.GetDevice()->createBuffer(indexBufferDesc);

    // WebGPU 要求 writeBuffer 的拷贝字节数满足4字节对齐
    // 当 uint16_t 索引个数为奇数时，原始字节数会是2的倍数但不是4的倍数，此处补一个索引位做 padding
    std::vector<uint16_t> paddedIndices = indexData;
    if ((paddedIndices.size() & 1u) != 0u) {
        paddedIndices.push_back(0);
    }
    const std::size_t writeSize = std::min<std::size_t>(
        paddedIndices.size() * sizeof(uint16_t),
        indexBufferDesc.size);
    ctx.GetQueue()->writeBuffer(*drawItem.indexBuffer, 0, paddedIndices.data(), writeSize);
    drawItem.indexBufferSize = indexBufferDesc.size;
    drawItem.indexCount      = static_cast<uint32_t>(indexData.size());
    drawItem.renderMode      = renderMode;
    drawItem.model           = object.Transform().Matrix();

    return drawItem;
}

// CPU侧计算逻辑场景
void Renderer3D::SyncScene(RenderContext& ctx, const std::vector<Object3D>& objects, const Camera& camera) {
    // 计算 MVP 矩阵
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

    // 初始化待绘制物体，如果物体的vertex和index没变化则不创建新的buffer
    m_drawItems.resize(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const Object3D& object = objects[i];
        const MeshData3D& mesh = object.Mesh();
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            m_drawItems[i] = {};
            continue;
        }

        DrawItem& drawItem = m_drawItems[i];
        const uint64_t vertexBytes = mesh.vertices.size() * sizeof(Vertex3D);
        const uint64_t indexBytes  = (object.RenderMode() == Object3D::ERenderMode::Wireframe)
            ? mesh.indices.size() * 2U * sizeof(uint16_t)
            : mesh.indices.size() * sizeof(uint16_t);
        const uint64_t alignedIndexBytes = (indexBytes + 3ull) & ~3ull;
        const uint32_t indexCount = (object.RenderMode() == Object3D::ERenderMode::Wireframe)
            ? static_cast<uint32_t>(mesh.indices.size() * 2U)
            : static_cast<uint32_t>(mesh.indices.size());

        const bool needUpload = !drawItem.vertexBuffer // 还没创建vertexBuffer
                                || !drawItem.indexBuffer // 还没创建indexBuffer
                                || drawItem.vertexBufferSize != vertexBytes // 顶点数据大小变了
                                || drawItem.indexBufferSize != alignedIndexBytes // 索引数据大小（4字节对齐后）变了
                                || drawItem.indexCount != indexCount // 索引数量变了
                                || drawItem.renderMode != object.RenderMode(); // 渲染模式变了
        if (needUpload) {
            DrawItem uploaded = UploadMeshToGpu(ctx, object);
            if (!uploaded.vertexBuffer || !uploaded.indexBuffer || uploaded.indexCount == 0) {
                drawItem = {};
                continue;
            }
            drawItem.vertexBuffer     = std::move(uploaded.vertexBuffer);
            drawItem.indexBuffer      = std::move(uploaded.indexBuffer);
            drawItem.vertexBufferSize = uploaded.vertexBufferSize;
            drawItem.indexBufferSize  = uploaded.indexBufferSize;
            drawItem.indexCount       = uploaded.indexCount;
            drawItem.renderMode       = uploaded.renderMode;
        }

        if (!drawItem.uniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size             = kSceneUniformSize;
            uniformBufferDesc.usage            = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            drawItem.uniformBuffer             = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
        if (!drawItem.bindGroup) {
            wgpu::BindGroupEntry binding{};
            binding.binding = 0;
            binding.buffer  = *drawItem.uniformBuffer;
            binding.offset  = 0;
            binding.size    = kSceneUniformSize;

            wgpu::BindGroupDescriptor bindGroupDesc{};
            bindGroupDesc.layout     = *m_bindGroupLayout;
            bindGroupDesc.entryCount = 1;
            bindGroupDesc.entries    = &binding;
            drawItem.bindGroup       = ctx.GetDevice()->createBindGroup(bindGroupDesc);
        }

        if (!drawItem.uniformBuffer || !drawItem.bindGroup) {
            drawItem = {};
            continue;
        }
        drawItem.model = object.Transform().Matrix();
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
        depthAttachment.stencilReadOnly   = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }

    wgpu::raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    Object3D::ERenderMode currentMode = Object3D::ERenderMode::Solid;
    if (m_solidPipeline) {
        renderPass->setPipeline(*m_solidPipeline);
    }

    // 将待绘制物体的MVP矩阵写入Uniform Buffer
    for (DrawItem& drawItem : m_drawItems) {
        if (drawItem.renderMode != currentMode) {
            if (drawItem.renderMode == Object3D::ERenderMode::Wireframe) {
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
            currentMode = drawItem.renderMode;
        }
        const SceneUniform uniform{
            .model      = drawItem.model,
            .view       = m_view,
            .projection = m_projection,
        };
        ctx.GetQueue()->writeBuffer(*drawItem.uniformBuffer, 0, &uniform, kSceneUniformSize);
        renderPass->setBindGroup(0, *drawItem.bindGroup, 0, nullptr);
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
    m_solidLayout     = {};
    m_wireframeLayout = {};
    m_bindGroupLayout = {};
    m_solidPipeline   = {};
    m_wireframePipeline = {};
    m_depthTexture    = {};
    m_depthView       = {};
    m_depthWidth      = 0;
    m_depthHeight     = 0;
    m_view            = glm::mat4(1.0f);
    m_projection      = glm::mat4(1.0f);
    m_drawItems.clear();
}
