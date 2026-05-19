#include "ShadowPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

static ShadowObjectUniformData BuildShadowObjectUniformData(const PreparedDrawItem& drawItem) {
    return ShadowObjectUniformData{
        .model = drawItem.model,
    };
}

void ShadowPass::Initialize(RenderContext& ctx) {
    auto pipeline = Scene3DPipelineFactory::CreateDirectionalShadowPipeline(ctx);
    m_sceneBindGroupLayout = std::move(pipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(pipeline.objectBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);
}

void ShadowPass::EnsureSceneResources(RenderContext& ctx) {
    if (!m_sceneResources.sceneUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(DirectionalShadowUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.sceneUniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void ShadowPass::EnsureObjectResources(RenderContext& ctx, const std::size_t objectCount) {
    if (m_objectResources.size() < objectCount) {
        m_objectResources.resize(objectCount);
    }

    for (std::size_t i = 0; i < objectCount; ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size = sizeof(ShadowObjectUniformData);
            uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            resources.objectUniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
    }
}

void ShadowPass::UpdateSceneResources(RenderContext& ctx, const PassContext& context) {
    if (!m_sceneResources.sceneUniformBuffer
        || !m_sceneBindGroupLayout
        || context.queue == nullptr
        || !context.directionalShadow.has_value()) {
        return;
    }

    context.queue->writeBuffer(
        *m_sceneResources.sceneUniformBuffer,
        0,
        &context.directionalShadow->uniformData,
        sizeof(DirectionalShadowUniformData));

    wgpu::BindGroupEntry sceneBinding{};
    sceneBinding.binding = 0;
    sceneBinding.buffer = *m_sceneResources.sceneUniformBuffer;
    sceneBinding.offset = 0;
    sceneBinding.size = sizeof(DirectionalShadowUniformData);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_sceneBindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &sceneBinding;
    m_sceneResources.sceneBindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);
}

void ShadowPass::UpdateObjectResources(RenderContext& ctx, const std::span<const PreparedDrawItem> drawItems) {
    if (!m_objectBindGroupLayout) {
        return;
    }

    for (std::size_t i = 0; i < drawItems.size(); ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            continue;
        }

        const ShadowObjectUniformData uniformData = BuildShadowObjectUniformData(drawItems[i]);
        ctx.GetQueue()->writeBuffer(
            *resources.objectUniformBuffer,
            0,
            &uniformData,
            sizeof(ShadowObjectUniformData));

        wgpu::BindGroupEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.buffer = *resources.objectUniformBuffer;
        objectBinding.offset = 0;
        objectBinding.size = sizeof(ShadowObjectUniformData);

        wgpu::BindGroupDescriptor objectBindGroupDesc{};
        objectBindGroupDesc.layout = *m_objectBindGroupLayout;
        objectBindGroupDesc.entryCount = 1;
        objectBindGroupDesc.entries = &objectBinding;
        resources.objectBindGroup = ctx.GetDevice()->createBindGroup(objectBindGroupDesc);
    }
}

void ShadowPass::Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) {
    if (!frame.encoder || !context.directionalShadow.has_value()) {
        return;
    }
    if (context.directionalShadow->uniformData.shadowParams.x <= 0.0f
        || context.directionalShadow->shadowMapView == nullptr) {
        return;
    }

    EnsureSceneResources(ctx);
    EnsureObjectResources(ctx, context.drawItems.size());
    UpdateSceneResources(ctx, context);
    UpdateObjectResources(ctx, context.drawItems);

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = context.directionalShadow->shadowMapView;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.depthReadOnly = false;
    depthAttachment.stencilReadOnly = true;

    wgpu::RenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 0;
    renderPassDesc.colorAttachments = nullptr;
    renderPassDesc.depthStencilAttachment = &depthAttachment;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    if (m_pipeline && m_sceneResources.sceneBindGroup) {
        renderPass->setPipeline(*m_pipeline);
        renderPass->setBindGroup(0, *m_sceneResources.sceneBindGroup, 0, nullptr);
        for (std::size_t i = 0; i < context.drawItems.size(); ++i) {
            const PreparedDrawItem& drawItem = context.drawItems[i];
            const wgpu::BindGroup objectBindGroup =
                i < m_objectResources.size() && m_objectResources[i].objectBindGroup
                    ? *m_objectResources[i].objectBindGroup
                    : nullptr;
            if (objectBindGroup == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            renderPass->setBindGroup(1, objectBindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}
