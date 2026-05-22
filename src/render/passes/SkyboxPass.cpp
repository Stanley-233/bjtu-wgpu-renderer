#include "SkyboxPass.h"

#include <glm/matrix.hpp>

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

namespace {

SkyboxUniformData BuildSkyboxUniformData(const PassContext& passCtx) {
    SkyboxUniformData uniformData{};
    if (!passCtx.camera.has_value()) {
        return uniformData;
    }

    const glm::mat4 viewRotation = glm::mat4(glm::mat3(passCtx.camera->view));
    uniformData.invViewRotation = glm::inverse(viewRotation);
    uniformData.invProjection = glm::inverse(passCtx.camera->projection);
    return uniformData;
}

} // namespace

void SkyboxPass::Initialize(RenderContext& renderCtx, const wgpu::TextureFormat colorTargetFormat) {
    auto pipeline = Scene3DPipelineFactory::CreateSkyboxPipeline(renderCtx, colorTargetFormat);
    m_bindGroupLayout = std::move(pipeline.skyboxBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(SkyboxUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    m_uniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
}

void SkyboxPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.encoder || frame.sceneColorView == nullptr) {
        return;
    }

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frame.sceneColorView;
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
    if (!m_pipeline
        || !m_bindGroupLayout
        || !m_uniformBuffer
        || passCtx.queue == nullptr
        || passCtx.skybox == nullptr
        || !passCtx.camera.has_value()
        || !passCtx.skybox->cubemapCubeView
        || !passCtx.skybox->sampler) {
        renderPass->end();
        return;
    }

    const SkyboxUniformData uniformData = BuildSkyboxUniformData(passCtx);
    passCtx.queue->writeBuffer(*m_uniformBuffer, 0, &uniformData, sizeof(uniformData));

    wgpu::BindGroupEntry bindings[3]{};
    bindings[0].binding = 0;
    bindings[0].buffer = *m_uniformBuffer;
    bindings[0].offset = 0;
    bindings[0].size = sizeof(SkyboxUniformData);
    bindings[1].binding = 1;
    bindings[1].textureView = *passCtx.skybox->cubemapCubeView;
    bindings[2].binding = 2;
    bindings[2].sampler = *passCtx.skybox->sampler;

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_bindGroupLayout;
    bindGroupDesc.entryCount = 3;
    bindGroupDesc.entries = bindings;
    wgpu::raii::BindGroup bindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);

    renderPass->setPipeline(*m_pipeline);
    renderPass->setBindGroup(0, *bindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();
}
