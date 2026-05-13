#include "ForwardPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

namespace {
constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;

struct SceneUniform {
    glm::mat4 model{1.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};
}

void ForwardPass::Initialize(RenderContext& ctx) {
    auto pipeline = Scene3DPipelineFactory::CreateForwardPipeline(ctx);
    m_bindGroupLayout = std::move(pipeline.bindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);
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
    if (m_pipeline && context.camera.has_value() && context.queue != nullptr) {
        renderPass->setPipeline(*m_pipeline);
        for (const PreparedDrawItem& drawItem : context.drawItems) {
            if (drawItem.renderMode != Object3D::ERenderMode::Solid
                || drawItem.bindGroup == nullptr
                || drawItem.uniformBuffer == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            const SceneUniform uniform{
                .model = drawItem.model,
                .view = context.camera->view,
                .projection = context.camera->projection,
            };
            context.queue->writeBuffer(drawItem.uniformBuffer, 0, &uniform, kSceneUniformSize);
            renderPass->setBindGroup(0, drawItem.bindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetBindGroupLayout() const {
    return m_bindGroupLayout;
}
