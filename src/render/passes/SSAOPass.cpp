#include "SSAOPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

void SSAOPass::Initialize(RenderContext& renderCtx) {
    auto pipeline = Scene3DPipelineFactory::CreateSsaoPipeline(renderCtx);
    m_ssaoBindGroupLayout = std::move(pipeline.ssaoBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);
}

void SSAOPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    (void)passCtx;
    if (!frame.encoder || frame.sceneDepthView == nullptr || frame.sceneAoView == nullptr || !m_pipeline
        || !m_ssaoBindGroupLayout) {
        return;
    }

    wgpu::BindGroupEntry ssaoBindGroupEntries[2]{};
    ssaoBindGroupEntries[0].binding = 0;
    ssaoBindGroupEntries[0].textureView = frame.sceneDepthView;
    ssaoBindGroupEntries[1].binding = 1;
    ssaoBindGroupEntries[1].textureView = frame.sceneNormalView;

    wgpu::BindGroupDescriptor ssaoBindGroupDesc{};
    ssaoBindGroupDesc.layout = *m_ssaoBindGroupLayout;
    ssaoBindGroupDesc.entryCount = 2;
    ssaoBindGroupDesc.entries = ssaoBindGroupEntries;

    wgpu::raii::BindGroup ssaoBindGroup = renderCtx.GetDevice()->createBindGroup(ssaoBindGroupDesc);

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
    renderPass->setBindGroup(0, *ssaoBindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();
}
