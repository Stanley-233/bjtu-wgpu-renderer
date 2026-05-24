#include "ForwardOpaquePass.h"

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
    uniformData.lightCounts = glm::uvec4{
        passCtx.lights.directionalLightCount,
        passCtx.lights.pointLightCount,
        passCtx.lights.spotLightCount,
        0U,
    };
    uniformData.directionalLight = passCtx.lights.directionalLight;
    uniformData.pointLights = passCtx.lights.pointLights;
    uniformData.spotLights = passCtx.lights.spotLights;
    return uniformData;
}

static DirectionalShadowUniformData BuildDirectionalShadowUniformData(const PassContext& passCtx) {
    if (passCtx.directionalShadow.has_value()) {
        return passCtx.directionalShadow->uniformData;
    }
    return DirectionalShadowUniformData{};
}

void ForwardOpaquePass::Initialize(RenderContext& renderCtx, const wgpu::TextureFormat colorTargetFormat) {
    auto unlitPipeline = Scene3DPipelineFactory::CreateUnlitForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::Back);
    m_unlitSceneBindGroupLayout = std::move(unlitPipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(unlitPipeline.objectBindGroupLayout);
    m_materialBindGroupLayout = std::move(unlitPipeline.materialBindGroupLayout);
    m_layout = std::move(unlitPipeline.layout);
    m_unlitPipelineSingleSided = std::move(unlitPipeline.pipeline);

    auto unlitDoubleSidedPipeline = Scene3DPipelineFactory::CreateUnlitForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::None);
    m_unlitPipelineDoubleSided = std::move(unlitDoubleSidedPipeline.pipeline);

    auto blinnPhongPipeline = Scene3DPipelineFactory::CreateBlinnPhongForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::Back);
    m_litSceneBindGroupLayout = std::move(blinnPhongPipeline.sceneBindGroupLayout);
    m_blinnPhongPipelineSingleSided = std::move(blinnPhongPipeline.pipeline);

    auto blinnPhongDoubleSidedPipeline = Scene3DPipelineFactory::CreateBlinnPhongForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::None);
    m_blinnPhongPipelineDoubleSided = std::move(blinnPhongDoubleSidedPipeline.pipeline);

    wgpu::SamplerDescriptor samplerDesc{};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
    samplerDesc.maxAnisotropy = 1;
    m_sceneAoSampler = renderCtx.GetDevice()->createSampler(samplerDesc);
}

void ForwardOpaquePass::EnsureSceneResources(RenderContext& renderCtx) {
    if (!m_sceneResources.sceneUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(SceneUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.sceneUniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
    }
    if (!m_sceneResources.directionalShadowUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(DirectionalShadowUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.directionalShadowUniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void ForwardOpaquePass::EnsureObjectResources(RenderContext& renderCtx, const std::size_t objectCount) {
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

void ForwardOpaquePass::UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx) {
    if (!m_sceneResources.sceneUniformBuffer
        || !m_unlitSceneBindGroupLayout
        || passCtx.queue == nullptr) {
        return;
    }

    const SceneUniformData uniformData = BuildSceneUniformData(passCtx);
    passCtx.queue->writeBuffer(
        *m_sceneResources.sceneUniformBuffer,
        0,
        &uniformData,
        sizeof(SceneUniformData));

    wgpu::BindGroupEntry unlitBinding{};
    unlitBinding.binding = 0;
    unlitBinding.buffer = *m_sceneResources.sceneUniformBuffer;
    unlitBinding.offset = 0;
    unlitBinding.size = sizeof(SceneUniformData);

    wgpu::BindGroupDescriptor unlitBindGroupDesc{};
    unlitBindGroupDesc.layout = *m_unlitSceneBindGroupLayout;
    unlitBindGroupDesc.entryCount = 1;
    unlitBindGroupDesc.entries = &unlitBinding;
    m_sceneResources.unlitSceneBindGroup = renderCtx.GetDevice()->createBindGroup(unlitBindGroupDesc);

    if (!m_sceneResources.directionalShadowUniformBuffer
        || !m_litSceneBindGroupLayout
        || !passCtx.directionalShadow.has_value()
        || !m_sceneAoSampler
        || passCtx.sceneAoView == nullptr
        || passCtx.directionalShadow->shadowMapView == nullptr
        || passCtx.directionalShadow->shadowSampler == nullptr) {
        m_sceneResources.litSceneBindGroup = {};
        return;
    }

    const DirectionalShadowUniformData directionalShadowUniformData = BuildDirectionalShadowUniformData(passCtx);
    passCtx.queue->writeBuffer(
        *m_sceneResources.directionalShadowUniformBuffer,
        0,
        &directionalShadowUniformData,
        sizeof(DirectionalShadowUniformData));

    wgpu::BindGroupEntry bindings[6]{};
    bindings[0].binding = 0;
    bindings[0].buffer = *m_sceneResources.sceneUniformBuffer;
    bindings[0].offset = 0;
    bindings[0].size = sizeof(SceneUniformData);
    bindings[1].binding = 1;
    bindings[1].buffer = *m_sceneResources.directionalShadowUniformBuffer;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(DirectionalShadowUniformData);
    bindings[2].binding = 2;
    bindings[2].textureView = passCtx.directionalShadow->shadowMapView;
    bindings[3].binding = 3;
    bindings[3].sampler = passCtx.directionalShadow->shadowSampler;
    bindings[4].binding = 4;
    bindings[4].textureView = passCtx.sceneAoView;
    bindings[5].binding = 5;
    bindings[5].sampler = *m_sceneAoSampler;

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_litSceneBindGroupLayout;
    bindGroupDesc.entryCount = 6;
    bindGroupDesc.entries = bindings;
    m_sceneResources.litSceneBindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);
}

void ForwardOpaquePass::UpdateObjectResources(RenderContext& renderCtx, const std::span<const PreparedDrawItem> drawItems) {
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

        wgpu::BindGroupEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.buffer = *resources.objectUniformBuffer;
        objectBinding.offset = 0;
        objectBinding.size = sizeof(ObjectUniformData);

        wgpu::BindGroupDescriptor objectBindGroupDesc{};
        objectBindGroupDesc.layout = *m_objectBindGroupLayout;
        objectBindGroupDesc.entryCount = 1;
        objectBindGroupDesc.entries = &objectBinding;
        resources.objectBindGroup = renderCtx.GetDevice()->createBindGroup(objectBindGroupDesc);
    }
}

void ForwardOpaquePass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.encoder || frame.sceneColorView == nullptr) {
        return;
    }

    EnsureSceneResources(renderCtx);
    EnsureObjectResources(renderCtx, passCtx.drawItems.size());
    UpdateSceneResources(renderCtx, passCtx);
    UpdateObjectResources(renderCtx, passCtx.drawItems);

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frame.sceneColorView;
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
    if (frame.sceneDepthView != nullptr) {
        depthAttachment.view = frame.sceneDepthView;
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
    for (std::size_t i = 0; i < passCtx.drawItems.size(); ++i) {
        const PreparedDrawItem& drawItem = passCtx.drawItems[i];
        const wgpu::RenderPipeline pipeline = SelectPipeline(drawItem.shadingModel, drawItem.doubleSided);
        const wgpu::BindGroup sceneBindGroup = SelectSceneBindGroup(drawItem.shadingModel);
        const wgpu::BindGroup objectBindGroup =
            i < m_objectResources.size() && m_objectResources[i].objectBindGroup
                ? *m_objectResources[i].objectBindGroup
                : nullptr;
        if (pipeline == nullptr
            || sceneBindGroup == nullptr
            || objectBindGroup == nullptr
            || drawItem.forwardMaterialBindGroup == nullptr
            || drawItem.vertexBuffer == nullptr
            || drawItem.indexBuffer == nullptr
            || drawItem.indexCount == 0) {
            continue;
        }

        renderPass->setPipeline(pipeline);
        renderPass->setBindGroup(0, sceneBindGroup, 0, nullptr);
        renderPass->setBindGroup(1, objectBindGroup, 0, nullptr);
        renderPass->setBindGroup(2, drawItem.forwardMaterialBindGroup, 0, nullptr);
        renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
        renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
        renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
    }
    renderPass->end();
}

const wgpu::raii::BindGroupLayout& ForwardOpaquePass::GetObjectBindGroupLayout() const {
    return m_objectBindGroupLayout;
}

const wgpu::raii::BindGroupLayout& ForwardOpaquePass::GetMaterialBindGroupLayout() const {
    return m_materialBindGroupLayout;
}

wgpu::BindGroup ForwardOpaquePass::SelectSceneBindGroup(const EMaterialShadingModel shadingModel) const {
    switch (shadingModel) {
    case EMaterialShadingModel::Unlit:
        return m_sceneResources.unlitSceneBindGroup ? *m_sceneResources.unlitSceneBindGroup : nullptr;
    case EMaterialShadingModel::BlinnPhong:
        return m_sceneResources.litSceneBindGroup ? *m_sceneResources.litSceneBindGroup : nullptr;
    case EMaterialShadingModel::Pbr:
        return nullptr;
    }
    return nullptr;
}

wgpu::RenderPipeline ForwardOpaquePass::SelectPipeline(const EMaterialShadingModel shadingModel, const bool doubleSided) const {
    switch (shadingModel) {
    case EMaterialShadingModel::Unlit:
        return doubleSided
                   ? (m_unlitPipelineDoubleSided ? *m_unlitPipelineDoubleSided : nullptr)
                   : (m_unlitPipelineSingleSided ? *m_unlitPipelineSingleSided : nullptr);
    case EMaterialShadingModel::BlinnPhong:
        return doubleSided
                   ? (m_blinnPhongPipelineDoubleSided ? *m_blinnPhongPipelineDoubleSided : nullptr)
                   : (m_blinnPhongPipelineSingleSided ? *m_blinnPhongPipelineSingleSided : nullptr);
    case EMaterialShadingModel::Pbr:
        return nullptr;
    }
    return nullptr;
}
