#include "ForwardPass.h"

#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

void ForwardPass::Initialize(RenderContext& ctx) {
    auto pipeline = Scene3DPipelineFactory::CreateUnlitForwardPipeline(ctx);
    m_sceneBindGroupLayout = std::move(pipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(pipeline.objectBindGroupLayout);
    m_materialBindGroupLayout = std::move(pipeline.materialBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_unlitPipeline = std::move(pipeline.pipeline);
}

void ForwardPass::Render(RenderFrame& frame, const PassContext& context) {
    if (!frame.surfaceFrame.view || !frame.encoder) {
        return;
    }

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = *frame.surfaceFrame.view;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = frame.clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    if (frame.depthView != nullptr) {
        depthAttachment.view = frame.depthView;
        depthAttachment.depthClearValue = 1.0f;
        depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthReadOnly = false;
        depthAttachment.stencilReadOnly = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    if (context.sceneBindGroup != nullptr) {
        renderPass->setBindGroup(0, context.sceneBindGroup, 0, nullptr);
        for (const PreparedDrawItem& drawItem : context.drawItems) {
            const wgpu::RenderPipeline pipeline = SelectPipeline(drawItem.shadingModel);
            if (pipeline == nullptr
                || drawItem.objectBindGroup == nullptr
                || drawItem.materialBindGroup == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            renderPass->setPipeline(pipeline);
            renderPass->setBindGroup(1, drawItem.objectBindGroup, 0, nullptr);
            renderPass->setBindGroup(2, drawItem.materialBindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetSceneBindGroupLayout() const {
    return m_sceneBindGroupLayout;
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetObjectBindGroupLayout() const {
    return m_objectBindGroupLayout;
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetMaterialBindGroupLayout() const {
    return m_materialBindGroupLayout;
}

wgpu::RenderPipeline ForwardPass::SelectPipeline(const EMaterialShadingModel shadingModel) const {
    switch (shadingModel) {
    case EMaterialShadingModel::Unlit:
        return m_unlitPipeline ? *m_unlitPipeline : nullptr;
    case EMaterialShadingModel::Lambert:
        // TODO: 接入 Lambert 前向渲染管线选择
        return nullptr;
    }
    return nullptr;
}
