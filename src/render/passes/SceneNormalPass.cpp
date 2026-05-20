#include "SceneNormalPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

static SceneUniformData BuildSceneUniformData(const PassContext& passCtx) {
    SceneUniformData uniformData{};
    if (passCtx.camera.has_value()) {
        uniformData.view = passCtx.camera->view;
        uniformData.projection = passCtx.camera->projection;
        uniformData.cameraPosition = glm::vec4{passCtx.camera->position, 1.0f};
    }
    return uniformData;
}

void SceneNormalPass::Initialize(RenderContext& renderCtx) {
    auto pipeline = Scene3DPipelineFactory::CreateSceneNormalPipeline(renderCtx, wgpu::TextureFormat::RGBA16Float);
    m_sceneBindGroupLayout = std::move(pipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(pipeline.objectBindGroupLayout);
    m_layout = std::move(pipeline.layout);
    m_pipeline = std::move(pipeline.pipeline);
}

void SceneNormalPass::EnsureSceneResources(RenderContext& renderCtx) {
    if (!m_sceneResources.sceneUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(SceneUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.sceneUniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void SceneNormalPass::EnsureObjectResources(RenderContext& renderCtx, const std::size_t objectCount) {
    if (m_objectResources.size() < objectCount) {
        m_objectResources.resize(objectCount);
    }

    for (std::size_t i = 0; i < objectCount; ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size = sizeof(ObjectUniformData);
            uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            resources.objectUniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
        }
    }
}

void SceneNormalPass::UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx) {
    if (!m_sceneResources.sceneUniformBuffer || !m_sceneBindGroupLayout || passCtx.queue == nullptr) {
        return;
    }

    const SceneUniformData uniformData = BuildSceneUniformData(passCtx);
    passCtx.queue->writeBuffer(
        *m_sceneResources.sceneUniformBuffer,
        0,
        &uniformData,
        sizeof(SceneUniformData));

    wgpu::BindGroupEntry binding{};
    binding.binding = 0;
    binding.buffer = *m_sceneResources.sceneUniformBuffer;
    binding.offset = 0;
    binding.size = sizeof(SceneUniformData);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_sceneBindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    m_sceneResources.sceneBindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);
}

void SceneNormalPass::UpdateObjectResources(RenderContext& renderCtx, const std::span<const PreparedDrawItem> drawItems) {
    if (!m_objectBindGroupLayout) {
        return;
    }

    for (std::size_t i = 0; i < drawItems.size(); ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            continue;
        }

        const PreparedDrawItem& drawItem = drawItems[i];
        renderCtx.GetQueue()->writeBuffer(
            *resources.objectUniformBuffer,
            0,
            &drawItem.objectUniformData,
            sizeof(ObjectUniformData));

        wgpu::BindGroupEntry binding{};
        binding.binding = 0;
        binding.buffer = *resources.objectUniformBuffer;
        binding.offset = 0;
        binding.size = sizeof(ObjectUniformData);

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = *m_objectBindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;
        resources.objectBindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);
    }
}

void SceneNormalPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.encoder || frame.sceneDepthView == nullptr || frame.sceneNormalView == nullptr || !m_pipeline) {
        return;
    }

    EnsureSceneResources(renderCtx);
    EnsureObjectResources(renderCtx, passCtx.drawItems.size());
    UpdateSceneResources(renderCtx, passCtx);
    UpdateObjectResources(renderCtx, passCtx.drawItems);

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frame.sceneNormalView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 0.0};
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = frame.sceneDepthView;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Undefined;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Undefined;
    depthAttachment.depthReadOnly = true;
    depthAttachment.stencilReadOnly = true;

    wgpu::RenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = &depthAttachment;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    if (m_sceneResources.sceneBindGroup) {
        renderPass->setPipeline(*m_pipeline);
        renderPass->setBindGroup(0, *m_sceneResources.sceneBindGroup, 0, nullptr);
        for (std::size_t i = 0; i < passCtx.drawItems.size(); ++i) {
            const PreparedDrawItem& drawItem = passCtx.drawItems[i];
            const wgpu::BindGroup objectBindGroup =
                i < m_objectResources.size() && m_objectResources[i].objectBindGroup
                    ? *m_objectResources[i].objectBindGroup
                    : nullptr;
            if (objectBindGroup == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            renderPass->setBindGroup(1, objectBindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}
