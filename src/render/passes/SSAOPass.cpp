#include "SSAOPass.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"
#include "render/scene/RenderUniformData.h"

constexpr float kDefaultSsaoRadius = 0.15f;
constexpr float kDefaultSsaoBias = 0.025f;
constexpr float kDefaultSsaoPower = 1.5f;
constexpr float kDefaultSsaoSampleCount = 16.0f;

static SsaoUniformData BuildSsaoUniformData(const PassContext& passCtx) {
    SsaoUniformData uniformData{};
    if (!passCtx.camera.has_value()) {
        return uniformData;
    }

    const float viewportWidth = static_cast<float>(std::max(passCtx.viewportWidth, 1));
    const float viewportHeight = static_cast<float>(std::max(passCtx.viewportHeight, 1));

    uniformData.projection = passCtx.camera->projection;
    uniformData.invProjection = glm::inverse(passCtx.camera->projection);
    uniformData.viewportSizeAndRadius = glm::vec4(viewportWidth, viewportHeight, kDefaultSsaoRadius, 0.0f);
    uniformData.aoParams = glm::vec4(kDefaultSsaoBias, kDefaultSsaoPower, kDefaultSsaoSampleCount, 0.0f);
    return uniformData;
}

void SSAOPass::Initialize(RenderContext& renderCtx) {
    auto pipeline = Scene3DPipelineFactory::CreateSsaoPipeline(renderCtx);
    m_ssaoBindGroupLayout = std::move(pipeline.ssaoBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(SsaoUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    m_uniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
}

void SSAOPass::SetEnabled(const bool enabled) {
    m_enabled = enabled;
}

void SSAOPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.encoder || frame.sceneAoView == nullptr || !m_pipeline) {
        return;
    }

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
    if (!m_enabled) {
        renderPass->end();
        return;
    }

    if (frame.sceneDepthView == nullptr
        || frame.sceneNormalView == nullptr
        || !m_ssaoBindGroupLayout
        || !m_uniformBuffer
        || passCtx.queue == nullptr
        || !passCtx.camera.has_value()) {
        renderPass->end();
        return;
    }

    const SsaoUniformData uniformData = BuildSsaoUniformData(passCtx);
    passCtx.queue->writeBuffer(
        *m_uniformBuffer,
        0,
        &uniformData,
        sizeof(SsaoUniformData));

    wgpu::BindGroupEntry ssaoBindGroupEntries[3]{};
    ssaoBindGroupEntries[0].binding = 0;
    ssaoBindGroupEntries[0].textureView = frame.sceneDepthView;
    ssaoBindGroupEntries[1].binding = 1;
    ssaoBindGroupEntries[1].textureView = frame.sceneNormalView;
    ssaoBindGroupEntries[2].binding = 2;
    ssaoBindGroupEntries[2].buffer = *m_uniformBuffer;
    ssaoBindGroupEntries[2].offset = 0;
    ssaoBindGroupEntries[2].size = sizeof(SsaoUniformData);

    wgpu::BindGroupDescriptor ssaoBindGroupDesc{};
    ssaoBindGroupDesc.layout = *m_ssaoBindGroupLayout;
    ssaoBindGroupDesc.entryCount = 3;
    ssaoBindGroupDesc.entries = ssaoBindGroupEntries;

    wgpu::raii::BindGroup ssaoBindGroup = renderCtx.GetDevice()->createBindGroup(ssaoBindGroupDesc);

    renderPass->setPipeline(*m_pipeline);
    renderPass->setBindGroup(0, *ssaoBindGroup, 0, nullptr);
    renderPass->draw(3, 1, 0, 0);
    renderPass->end();
}
