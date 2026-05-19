#include "ForwardPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

namespace {
SceneUniformData BuildSceneUniformData(const PassContext& context) {
    SceneUniformData uniformData{};
    if (context.camera.has_value()) {
        uniformData.view = context.camera->view;
        uniformData.projection = context.camera->projection;
        uniformData.cameraPosition = glm::vec4{context.camera->position, 1.0f};
    }
    uniformData.lightCounts = glm::uvec4{
        context.lights.directionalLightCount,
        context.lights.pointLightCount,
        context.lights.spotLightCount,
        0U,
    };
    uniformData.directionalLight = context.lights.directionalLight;
    uniformData.pointLights = context.lights.pointLights;
    uniformData.spotLights = context.lights.spotLights;
    return uniformData;
}
} // namespace

void ForwardPass::Initialize(RenderContext& ctx) {
    auto unlitPipeline = Scene3DPipelineFactory::CreateUnlitForwardPipeline(ctx);
    m_sceneBindGroupLayout = std::move(unlitPipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(unlitPipeline.objectBindGroupLayout);
    m_materialBindGroupLayout = std::move(unlitPipeline.materialBindGroupLayout);
    m_layout = std::move(unlitPipeline.layout);
    m_unlitPipeline = std::move(unlitPipeline.pipeline);

    auto lambertPipeline = Scene3DPipelineFactory::CreateLambertForwardPipeline(ctx);
    m_lambertPipeline = std::move(lambertPipeline.pipeline);
}

void ForwardPass::EnsureSceneResources(RenderContext& ctx) {
    if (!m_sceneResources.sceneUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(SceneUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.sceneUniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void ForwardPass::EnsureObjectResources(RenderContext& ctx, const std::size_t objectCount) {
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
            resources.objectUniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
    }
}

void ForwardPass::UpdateSceneResources(RenderContext& ctx, const PassContext& context) {
    if (!m_sceneResources.sceneUniformBuffer || !m_sceneBindGroupLayout || context.queue == nullptr) {
        return;
    }

    const SceneUniformData uniformData = BuildSceneUniformData(context);
    context.queue->writeBuffer(
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
    m_sceneResources.sceneBindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);
}

void ForwardPass::UpdateObjectResources(RenderContext& ctx, const std::span<const PreparedDrawItem> drawItems) {
    if (!m_objectBindGroupLayout) {
        return;
    }

    for (std::size_t i = 0; i < drawItems.size(); ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            continue;
        }

        const PreparedDrawItem& drawItem = drawItems[i];
        ctx.GetQueue()->writeBuffer(
            *resources.objectUniformBuffer,
            0,
            &drawItem.objectUniformData,
            sizeof(ObjectUniformData));

        wgpu::BindGroupEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.buffer = *resources.objectUniformBuffer;
        objectBinding.offset = 0;
        objectBinding.size = sizeof(ObjectUniformData);

        wgpu::BindGroupDescriptor objectBindGroupDesc{};
        objectBindGroupDesc.layout = *m_objectBindGroupLayout;
        objectBindGroupDesc.entryCount = 1;
        objectBindGroupDesc.entries = &objectBinding;
        resources.objectBindGroup = ctx.GetDevice()->createBindGroup(objectBindGroupDesc);
    }
}

void ForwardPass::Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) {
    if (!frame.surfaceFrame.view || !frame.encoder) {
        return;
    }

    EnsureSceneResources(ctx);
    EnsureObjectResources(ctx, context.drawItems.size());
    UpdateSceneResources(ctx, context);
    UpdateObjectResources(ctx, context.drawItems);

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
    if (m_sceneResources.sceneBindGroup) {
        renderPass->setBindGroup(0, *m_sceneResources.sceneBindGroup, 0, nullptr);
        for (std::size_t i = 0; i < context.drawItems.size(); ++i) {
            const PreparedDrawItem& drawItem = context.drawItems[i];
            const wgpu::RenderPipeline pipeline = SelectPipeline(drawItem.shadingModel);
            const wgpu::BindGroup objectBindGroup =
                i < m_objectResources.size() && m_objectResources[i].objectBindGroup
                    ? *m_objectResources[i].objectBindGroup
                    : nullptr;
            if (pipeline == nullptr
                || objectBindGroup == nullptr
                || drawItem.materialBindGroup == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            renderPass->setPipeline(pipeline);
            renderPass->setBindGroup(1, objectBindGroup, 0, nullptr);
            renderPass->setBindGroup(2, drawItem.materialBindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetSceneBindGroupLayout() const {
    return m_sceneBindGroupLayout;
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetObjectBindGroupLayout() const {
    return m_objectBindGroupLayout;
}

const wgpu::raii::BindGroupLayout& ForwardPass::GetMaterialBindGroupLayout() const {
    return m_materialBindGroupLayout;
}

wgpu::RenderPipeline ForwardPass::SelectPipeline(const EMaterialShadingModel shadingModel) const {
    switch (shadingModel) {
    case EMaterialShadingModel::Unlit:
        return m_unlitPipeline ? *m_unlitPipeline : nullptr;
    case EMaterialShadingModel::Lambert:
        return m_lambertPipeline ? *m_lambertPipeline : nullptr;
    }
    return nullptr;
}
