#include "WireframePass.h"

#include "render/PipelineLibrary.h"
#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"

namespace {
constexpr std::size_t kSceneUniformSize = sizeof(glm::mat4) * 3;

struct SceneUniform {
    glm::mat4 model{1.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};
}

void WireframePass::Initialize(RenderContext& ctx) {
    auto wireframePipeline = PipelineLibrary::CreateColor3DWireframe(ctx);
    auto depthPrepassPipeline = PipelineLibrary::CreateColor3DWireframeDepthPrepass(ctx);
    m_wireframeLayout = std::move(wireframePipeline.layout);
    m_wireframePipeline = std::move(wireframePipeline.pipeline);
    m_depthPrepassLayout = std::move(depthPrepassPipeline.layout);
    m_depthPrepassPipeline = std::move(depthPrepassPipeline.pipeline);
}

void WireframePass::Render(RenderFrame& frame, const PassContext& context) {
    if (!frame.surfaceView || !frame.encoder || !context.camera.has_value() || context.queue == nullptr) {
        return;
    }

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = *frame.surfaceView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = wgpu::LoadOp::Load;
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
        depthAttachment.depthLoadOp = wgpu::LoadOp::Load;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthReadOnly = false;
        depthAttachment.stencilReadOnly = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    for (const PreparedDrawItem& drawItem : context.drawItems) {
        if (drawItem.renderMode != Object3D::ERenderMode::Wireframe
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

        if (m_depthPrepassPipeline
            && drawItem.wireframeDepthIndexBuffer != nullptr
            && drawItem.wireframeDepthIndexCount > 0) {
            renderPass->setPipeline(*m_depthPrepassPipeline);
            renderPass->setIndexBuffer(
                drawItem.wireframeDepthIndexBuffer,
                wgpu::IndexFormat::Uint16,
                0,
                drawItem.wireframeDepthIndexBufferSize);
            renderPass->drawIndexed(drawItem.wireframeDepthIndexCount, 1, 0, 0, 0);
        }

        if (!m_wireframePipeline) {
            continue;
        }
        renderPass->setPipeline(*m_wireframePipeline);
        renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
        renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
    }
    renderPass->end();
}
