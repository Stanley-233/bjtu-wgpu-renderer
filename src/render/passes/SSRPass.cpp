#include "SSRPass.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

static SsrUniformData BuildSsrUniformData(const PassContext& passCtx, const SsrSettings& settings) {
    SsrUniformData uniformData{};
    if (!passCtx.camera.has_value()) {
        return uniformData;
    }

    const float viewportWidth = static_cast<float>(std::max(passCtx.viewportWidth, 1));
    const float viewportHeight = static_cast<float>(std::max(passCtx.viewportHeight, 1));
    uniformData.projection = passCtx.camera->projection;
    uniformData.invProjection = glm::inverse(passCtx.camera->projection);
    uniformData.viewport = glm::vec4(viewportWidth, viewportHeight, 0.0f, 0.0f);
    uniformData.params = glm::vec4(settings.strength, settings.maxDistance, settings.thickness, 24.0f);
    return uniformData;
}

void SSRPass::Initialize(RenderContext& renderCtx, const wgpu::TextureFormat colorTargetFormat) {
    auto pipeline = Scene3DPipelineFactory::CreateSsrPipeline(renderCtx, colorTargetFormat);
    m_bindGroupLayout = std::move(pipeline.bindGroupLayout);
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
    m_sceneSampler = renderCtx.GetDevice()->createSampler(samplerDesc);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(SsrUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    m_uniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
}

void SSRPass::SetSettings(const SsrSettings& settings) {
    m_settings = settings;
}

void SSRPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!m_settings.enabled) {
        return;
    }

    if (!frame.encoder
        || frame.sceneDepthView == nullptr
        || frame.sceneNormalView == nullptr
        || frame.sceneReflectivityView == nullptr
        || frame.sceneSsrColorView == nullptr
        || frame.postProcessColorView == nullptr
        || !m_bindGroupLayout
        || !m_sceneSampler
        || !m_pipeline
        || !m_uniformBuffer
        || passCtx.queue == nullptr
        || !passCtx.camera.has_value()) {
        return;
    }

    const SsrUniformData uniformData = BuildSsrUniformData(passCtx, m_settings);
    passCtx.queue->writeBuffer(*m_uniformBuffer, 0, &uniformData, sizeof(uniformData));

    wgpu::BindGroupEntry entries[6]{};
    entries[0].binding = 0;
    entries[0].textureView = frame.sceneDepthView;
    entries[1].binding = 1;
    entries[1].textureView = frame.sceneNormalView;
    entries[2].binding = 2;
    entries[2].textureView = frame.sceneReflectivityView;
    entries[3].binding = 3;
    entries[3].textureView = frame.postProcessColorView;
    entries[4].binding = 4;
    entries[4].sampler = *m_sceneSampler;
    entries[5].binding = 5;
    entries[5].buffer = *m_uniformBuffer;
    entries[5].offset = 0;
    entries[5].size = sizeof(SsrUniformData);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_bindGroupLayout;
    bindGroupDesc.entryCount = 6;
    bindGroupDesc.entries = entries;
    wgpu::raii::BindGroup bindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frame.sceneSsrColorView;
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
    renderPass->setBindGroup(0, *bindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();

    frame.postProcessColorView = frame.sceneSsrColorView;
}
