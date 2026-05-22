#include "ToneMapPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

void ToneMapPass::Initialize(RenderContext& renderCtx) {
    auto pipeline = Scene3DPipelineFactory::CreateToneMapPipeline(renderCtx, renderCtx.GetSurfaceFormat());
    m_sceneColorBindGroupLayout = std::move(pipeline.sceneColorBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);

    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
    samplerDesc.maxAnisotropy = 1;
    m_sceneColorSampler = renderCtx.GetDevice()->createSampler(samplerDesc);
}

void ToneMapPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.surfaceFrame.view
        || !frame.encoder
        || !m_pipeline
        || !m_sceneColorBindGroupLayout
        || !m_sceneColorSampler
        || passCtx.sceneColorView == nullptr) {
        return;
    }

    wgpu::BindGroupEntry bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].textureView = passCtx.sceneColorView;
    bindings[1].binding = 1;
    bindings[1].sampler = *m_sceneColorSampler;

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_sceneColorBindGroupLayout;
    bindGroupDesc.entryCount = 2;
    bindGroupDesc.entries = bindings;
    wgpu::raii::BindGroup sceneColorBindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);

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
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    renderPass->setPipeline(*m_pipeline);
    renderPass->setBindGroup(0, *sceneColorBindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();
}
