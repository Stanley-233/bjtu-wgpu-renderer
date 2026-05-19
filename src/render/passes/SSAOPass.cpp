#include "SSAOPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

void SSAOPass::Initialize(RenderContext& ctx) {
    auto pipeline = Scene3DPipelineFactory::CreateSsaoPipeline(ctx);
    m_depthBindGroupLayout = std::move(pipeline.depthBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);
}

void SSAOPass::Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) {
    (void)context;
    if (!frame.encoder || frame.sceneDepthView == nullptr || frame.sceneAoView == nullptr || !m_pipeline
        || !m_depthBindGroupLayout) {
        return;
    }

    wgpu::BindGroupEntry binding{};
    binding.binding = 0;
    binding.textureView = frame.sceneDepthView;

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_depthBindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    wgpu::raii::BindGroup depthBindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frame.sceneAoView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = wgpu::Color{1.0, 1.0, 1.0, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    renderPass->setPipeline(*m_pipeline);
    renderPass->setBindGroup(0, *depthBindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();
}
