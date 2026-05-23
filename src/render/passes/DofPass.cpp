#include "DofPass.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

namespace {

DofUniformData BuildDofUniformData(const PassContext& passCtx, const DofSettings& settings, const glm::vec2 blurDirection) {
    DofUniformData uniformData{};
    if (!passCtx.camera.has_value()) {
        return uniformData;
    }

    const float viewportWidth = static_cast<float>(std::max(passCtx.viewportWidth, 1));
    const float viewportHeight = static_cast<float>(std::max(passCtx.viewportHeight, 1));
    uniformData.invProjection = glm::inverse(passCtx.camera->projection);
    uniformData.viewportAndBlur = glm::vec4(viewportWidth, viewportHeight, settings.maxBlurRadiusPixels, 0.0f);
    uniformData.focusParams = glm::vec4(
        settings.focusDistance,
        settings.focusRange,
        settings.debugPlaneHalfThickness,
        static_cast<float>(settings.debugMode));
    uniformData.blurDirection = glm::vec4(blurDirection, 0.0f, 0.0f);
    return uniformData;
}

wgpu::raii::BindGroup CreateCocBindGroup(
    RenderContext& renderCtx,
    const wgpu::BindGroupLayout layout,
    const wgpu::TextureView depthView,
    const wgpu::Buffer uniformBuffer) {
    wgpu::BindGroupEntry entries[2]{};
    entries[0].binding = 0;
    entries[0].textureView = depthView;
    entries[1].binding = 1;
    entries[1].buffer = uniformBuffer;
    entries[1].offset = 0;
    entries[1].size = sizeof(DofUniformData);

    wgpu::BindGroupDescriptor desc{};
    desc.layout = layout;
    desc.entryCount = 2;
    desc.entries = entries;
    return renderCtx.GetDevice()->createBindGroup(desc);
}

wgpu::raii::BindGroup CreateBlurBindGroup(
    RenderContext& renderCtx,
    const wgpu::BindGroupLayout layout,
    const wgpu::TextureView sceneColorView,
    const wgpu::TextureView cocView,
    const wgpu::TextureView depthView,
    const wgpu::Sampler colorSampler,
    const wgpu::Sampler cocSampler,
    const wgpu::Buffer uniformBuffer) {
    wgpu::BindGroupEntry entries[6]{};
    entries[0].binding = 0;
    entries[0].textureView = sceneColorView;
    entries[1].binding = 1;
    entries[1].textureView = cocView;
    entries[2].binding = 2;
    entries[2].sampler = colorSampler;
    entries[3].binding = 3;
    entries[3].sampler = cocSampler;
    entries[4].binding = 4;
    entries[4].buffer = uniformBuffer;
    entries[4].offset = 0;
    entries[4].size = sizeof(DofUniformData);
    entries[5].binding = 5;
    entries[5].textureView = depthView;

    wgpu::BindGroupDescriptor desc{};
    desc.layout = layout;
    desc.entryCount = 6;
    desc.entries = entries;
    return renderCtx.GetDevice()->createBindGroup(desc);
}

void WriteUniform(wgpu::Queue* queue, const DofUniformData& uniformData, const wgpu::Buffer buffer) {
    if (queue == nullptr) {
        return;
    }
    queue->writeBuffer(buffer, 0, &uniformData, sizeof(uniformData));
}

} // namespace

void DofPass::Initialize(RenderContext& renderCtx, const wgpu::TextureFormat colorTargetFormat) {
    auto cocPipeline = Scene3DPipelineFactory::CreateDofCocPipeline(renderCtx);
    m_cocBindGroupLayout = std::move(cocPipeline.bindGroupLayout);
    m_cocLayout = std::move(cocPipeline.layout);
    m_cocPipeline = std::move(cocPipeline.pipeline);

    auto blurPipeline = Scene3DPipelineFactory::CreateDofBlurPipeline(renderCtx, colorTargetFormat);
    m_blurBindGroupLayout = std::move(blurPipeline.bindGroupLayout);
    m_blurLayout = std::move(blurPipeline.layout);
    m_blurPipeline = std::move(blurPipeline.pipeline);

    wgpu::SamplerDescriptor linearSamplerDesc{};
    linearSamplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    linearSamplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    linearSamplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    linearSamplerDesc.magFilter = wgpu::FilterMode::Linear;
    linearSamplerDesc.minFilter = wgpu::FilterMode::Linear;
    linearSamplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
    linearSamplerDesc.maxAnisotropy = 1;
    m_colorSampler = renderCtx.GetDevice()->createSampler(linearSamplerDesc);
    m_cocSampler = renderCtx.GetDevice()->createSampler(linearSamplerDesc);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(DofUniformData);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    m_uniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
}

void DofPass::SetSettings(const DofSettings& settings) {
    m_settings = settings;
}

void DofPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!m_settings.enabled) {
        frame.postProcessColorView = frame.sceneColorView;
        return;
    }

    if (!frame.encoder
        || frame.sceneDepthView == nullptr
        || frame.sceneColorView == nullptr
        || frame.sceneCocView == nullptr
        || frame.sceneDofPingView == nullptr
        || frame.sceneDofColorView == nullptr
        || !m_cocPipeline
        || !m_blurPipeline
        || !m_cocBindGroupLayout
        || !m_blurBindGroupLayout
        || !m_colorSampler
        || !m_cocSampler
        || !m_uniformBuffer
        || passCtx.queue == nullptr
        || !passCtx.camera.has_value()) {
        frame.postProcessColorView = frame.sceneColorView;
        return;
    }

    {
        const DofUniformData uniformData = BuildDofUniformData(passCtx, m_settings, {0.0f, 0.0f});
        WriteUniform(passCtx.queue, uniformData, *m_uniformBuffer);
        wgpu::raii::BindGroup cocBindGroup = CreateCocBindGroup(
            renderCtx,
            *m_cocBindGroupLayout,
            frame.sceneDepthView,
            *m_uniformBuffer);

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = frame.sceneCocView;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 0.0};
#ifndef WEBGPU_BACKEND_WGPU
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
        renderPass->setPipeline(*m_cocPipeline);
        renderPass->setBindGroup(0, *cocBindGroup, 0, nullptr);
        renderPass->draw(3, 1, 0, 0);
        renderPass->end();
    }

    {
        const DofUniformData uniformData = BuildDofUniformData(passCtx, m_settings, {1.0f, 0.0f});
        WriteUniform(passCtx.queue, uniformData, *m_uniformBuffer);
        wgpu::raii::BindGroup blurBindGroup = CreateBlurBindGroup(
            renderCtx,
            *m_blurBindGroupLayout,
            frame.sceneColorView,
            frame.sceneCocView,
            frame.sceneDepthView,
            *m_colorSampler,
            *m_cocSampler,
            *m_uniformBuffer);

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = frame.sceneDofPingView;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
        renderPass->setPipeline(*m_blurPipeline);
        renderPass->setBindGroup(0, *blurBindGroup, 0, nullptr);
        renderPass->draw(3, 1, 0, 0);
        renderPass->end();
    }

    {
        const DofUniformData uniformData = BuildDofUniformData(passCtx, m_settings, {0.0f, 1.0f});
        WriteUniform(passCtx.queue, uniformData, *m_uniformBuffer);
        wgpu::raii::BindGroup blurBindGroup = CreateBlurBindGroup(
            renderCtx,
            *m_blurBindGroupLayout,
            frame.sceneDofPingView,
            frame.sceneCocView,
            frame.sceneDepthView,
            *m_colorSampler,
            *m_cocSampler,
            *m_uniformBuffer);

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = frame.sceneDofColorView;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
        renderPass->setPipeline(*m_blurPipeline);
        renderPass->setBindGroup(0, *blurBindGroup, 0, nullptr);
        renderPass->draw(3, 1, 0, 0);
        renderPass->end();
    }

    frame.postProcessColorView = frame.sceneDofColorView;
}
