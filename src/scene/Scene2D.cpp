#include "Scene2D.h"

#include <iostream>
#include <utility>
#include <vector>

#include "../render/PipelineLibrary.h"
#include "../render/RenderContext.h"
#include "../resource/ResourceManager.h"

using namespace wgpu;

void Scene2D::Initialize(RenderContext& ctx) {
    auto pipeline2D       = PipelineLibrary::CreateColor2D(ctx);
    m_bindGroupLayout     = std::move(pipeline2D.bindGroupLayout);
    m_layout              = std::move(pipeline2D.layout);
    m_pipeline            = std::move(pipeline2D.pipeline);
    InitializeBuffers(ctx);
    InitializeBindGroups(ctx);
}

void Scene2D::Update(const float dt) {
    (void)dt;
}

void Scene2D::Render(RenderContext& ctx) {
    const float t = static_cast<float>(glfwGetTime());
    ctx.GetQueue()->writeBuffer(*m_uniformBuffer, 0, &t, sizeof(float));

    raii::TextureView targetView = ctx.AcquireNextSurfaceView();
    if (!targetView) {
        return;
    }

    raii::CommandEncoder encoder = ctx.BeginFrame();

    RenderPassDescriptor      renderPassDesc            = {};
    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view                      = *targetView;
    renderPassColorAttachment.resolveTarget             = nullptr;
    renderPassColorAttachment.loadOp                    = LoadOp::Clear;
    renderPassColorAttachment.storeOp                   = StoreOp::Store;
    renderPassColorAttachment.clearValue                = WGPUColor{0.05, 0.05, 0.05, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    renderPassDesc.colorAttachmentCount   = 1;
    renderPassDesc.colorAttachments       = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites        = nullptr;

    raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    renderPass->setPipeline(*m_pipeline);
    renderPass->setVertexBuffer(0, *m_pointBuffer, 0, m_pointBuffer->getSize());
    renderPass->setIndexBuffer(*m_indexBuffer, IndexFormat::Uint16, 0, m_indexBuffer->getSize());
    renderPass->setBindGroup(0, *m_bindGroup, 0, nullptr);
    renderPass->drawIndexed(m_indexCount, 1, 0, 0, 0);
    renderPass->end();

    ctx.SubmitAndPresent(encoder);
}

const char* Scene2D::Name() const {
    return "Scene2D";
}

void Scene2D::InitializeBuffers(RenderContext& ctx) {
    std::vector<float>    pointData;
    std::vector<uint16_t> indexData;

    if (const bool success = ResourceManager::LoadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData); !success) {
        std::cerr << "Could not load geometry!" << std::endl;
        std::exit(1);
    }

    m_indexCount = static_cast<uint32_t>(indexData.size());

    BufferDescriptor bufferDesc;
    bufferDesc.size             = pointData.size() * sizeof(float);
    bufferDesc.usage            = BufferUsage::CopyDst | BufferUsage::Vertex;
    bufferDesc.mappedAtCreation = false;
    m_pointBuffer               = ctx.GetDevice()->createBuffer(bufferDesc);
    ctx.GetQueue()->writeBuffer(*m_pointBuffer, 0, pointData.data(), bufferDesc.size);

    bufferDesc.size  = indexData.size() * sizeof(uint16_t);
    bufferDesc.size  = (bufferDesc.size + 3) & ~3;
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    m_indexBuffer    = ctx.GetDevice()->createBuffer(bufferDesc);
    ctx.GetQueue()->writeBuffer(*m_indexBuffer, 0, indexData.data(), bufferDesc.size);

    bufferDesc.size             = 4 * sizeof(float);
    bufferDesc.usage            = BufferUsage::CopyDst | BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;
    m_uniformBuffer             = ctx.GetDevice()->createBuffer(bufferDesc);
    const float currentTime     = 1.0f;
    ctx.GetQueue()->writeBuffer(*m_uniformBuffer, 0, &currentTime, sizeof(float));
}

void Scene2D::InitializeBindGroups(RenderContext& ctx) {
    BindGroupEntry binding{};
    binding.binding = 0;
    binding.buffer  = *m_uniformBuffer;
    binding.offset  = 0;
    binding.size    = 4 * sizeof(float);

    BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout     = *m_bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries    = &binding;
    m_bindGroup              = ctx.GetDevice()->createBindGroup(bindGroupDesc);
}
