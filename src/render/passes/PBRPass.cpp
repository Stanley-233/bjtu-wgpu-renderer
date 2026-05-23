#include "PBRPass.h"

#include "render/RenderContext.h"
#include "render/frame/RenderFrame.h"
#include "render/pipelines/Scene3DPipelineFactory.h"

namespace {
SceneUniformData BuildSceneUniformData(const PassContext& passCtx) {
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

DirectionalShadowUniformData BuildDirectionalShadowUniformData(const PassContext& passCtx) {
    if (passCtx.directionalShadow.has_value()) {
        return passCtx.directionalShadow->uniformData;
    }
    return DirectionalShadowUniformData{};
}

PbrDebugUniformData BuildPbrDebugUniformData(const PassContext& passCtx) {
    return PbrDebugUniformData{
        .options = glm::uvec4{
            static_cast<uint32_t>(passCtx.pbrDebugView),
            0U,
            0U,
            0U,
        },
    };
}
} // namespace

void PBRPass::Initialize(RenderContext& renderCtx, const wgpu::TextureFormat colorTargetFormat) {
    auto pbrPipeline = Scene3DPipelineFactory::CreatePbrForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::Back);
    m_sceneBindGroupLayout = std::move(pbrPipeline.sceneBindGroupLayout);
    m_objectBindGroupLayout = std::move(pbrPipeline.objectBindGroupLayout);
    m_materialBindGroupLayout = std::move(pbrPipeline.materialBindGroupLayout);
    m_debugBindGroupLayout = std::move(pbrPipeline.debugBindGroupLayout);
    m_layout = std::move(pbrPipeline.layout);
    m_pipelineSingleSided = std::move(pbrPipeline.pipeline);

    auto pbrDoubleSidedPipeline = Scene3DPipelineFactory::CreatePbrForwardPipeline(
        renderCtx,
        colorTargetFormat,
        wgpu::CullMode::None);
    m_pipelineDoubleSided = std::move(pbrDoubleSidedPipeline.pipeline);

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

void PBRPass::EnsureSceneResources(RenderContext& renderCtx) {
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
    if (!m_sceneResources.debugUniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(PbrDebugUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.debugUniformBuffer = renderCtx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void PBRPass::EnsureObjectResources(RenderContext& renderCtx, const std::size_t objectCount) {
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

void PBRPass::UpdateSceneResources(RenderContext& renderCtx, const PassContext& passCtx) {
    if (!m_sceneResources.sceneUniformBuffer
        || !m_sceneResources.directionalShadowUniformBuffer
        || !m_sceneResources.debugUniformBuffer
        || !m_sceneBindGroupLayout
        || !m_debugBindGroupLayout
        || passCtx.queue == nullptr
        || !passCtx.directionalShadow.has_value()
        || !m_sceneAoSampler
        || passCtx.sceneAoView == nullptr
        || passCtx.directionalShadow->shadowMapView == nullptr
        || passCtx.directionalShadow->shadowSampler == nullptr) {
        return;
    }

    const SceneUniformData uniformData = BuildSceneUniformData(passCtx);
    const DirectionalShadowUniformData directionalShadowUniformData = BuildDirectionalShadowUniformData(passCtx);
    const PbrDebugUniformData debugUniformData = BuildPbrDebugUniformData(passCtx);
    passCtx.queue->writeBuffer(*m_sceneResources.sceneUniformBuffer, 0, &uniformData, sizeof(SceneUniformData));
    passCtx.queue->writeBuffer(
        *m_sceneResources.directionalShadowUniformBuffer,
        0,
        &directionalShadowUniformData,
        sizeof(DirectionalShadowUniformData));
    passCtx.queue->writeBuffer(
        *m_sceneResources.debugUniformBuffer,
        0,
        &debugUniformData,
        sizeof(PbrDebugUniformData));

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
    bindGroupDesc.layout = *m_sceneBindGroupLayout;
    bindGroupDesc.entryCount = 6;
    bindGroupDesc.entries = bindings;
    m_sceneResources.sceneBindGroup = renderCtx.GetDevice()->createBindGroup(bindGroupDesc);

    wgpu::BindGroupEntry debugBinding{};
    debugBinding.binding = 0;
    debugBinding.buffer = *m_sceneResources.debugUniformBuffer;
    debugBinding.offset = 0;
    debugBinding.size = sizeof(PbrDebugUniformData);

    wgpu::BindGroupDescriptor debugBindGroupDesc{};
    debugBindGroupDesc.layout = *m_debugBindGroupLayout;
    debugBindGroupDesc.entryCount = 1;
    debugBindGroupDesc.entries = &debugBinding;
    m_sceneResources.debugBindGroup = renderCtx.GetDevice()->createBindGroup(debugBindGroupDesc);
}

void PBRPass::UpdateObjectResources(RenderContext& renderCtx, const std::span<const PreparedDrawItem> drawItems) {
    if (!m_objectBindGroupLayout) {
        return;
    }

    for (std::size_t i = 0; i < drawItems.size(); ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.objectUniformBuffer) {
            continue;
        }

        renderCtx.GetQueue()->writeBuffer(
            *resources.objectUniformBuffer,
            0,
            &drawItems[i].objectUniformData,
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

bool PBRPass::HasPbrDrawItems(const std::span<const PreparedDrawItem> drawItems) const {
    for (const PreparedDrawItem& drawItem : drawItems) {
        if (drawItem.shadingModel == EMaterialShadingModel::Pbr) {
            return true;
        }
    }
    return false;
}

void PBRPass::Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) {
    if (!frame.encoder || frame.sceneColorView == nullptr || !HasPbrDrawItems(passCtx.drawItems)) {
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
    if (m_sceneResources.sceneBindGroup && m_sceneResources.debugBindGroup) {
        renderPass->setBindGroup(0, *m_sceneResources.sceneBindGroup, 0, nullptr);
        renderPass->setBindGroup(3, *m_sceneResources.debugBindGroup, 0, nullptr);
        for (std::size_t i = 0; i < passCtx.drawItems.size(); ++i) {
            const PreparedDrawItem& drawItem = passCtx.drawItems[i];
            const wgpu::RenderPipeline pipeline = SelectPipeline(drawItem.doubleSided);
            const wgpu::BindGroup objectBindGroup =
                i < m_objectResources.size() && m_objectResources[i].objectBindGroup
                    ? *m_objectResources[i].objectBindGroup
                    : nullptr;
            if (pipeline == nullptr
                || drawItem.shadingModel != EMaterialShadingModel::Pbr
                || objectBindGroup == nullptr
                || drawItem.pbrMaterialBindGroup == nullptr
                || drawItem.vertexBuffer == nullptr
                || drawItem.indexBuffer == nullptr
                || drawItem.indexCount == 0) {
                continue;
            }

            renderPass->setPipeline(pipeline);
            renderPass->setBindGroup(1, objectBindGroup, 0, nullptr);
            renderPass->setBindGroup(2, drawItem.pbrMaterialBindGroup, 0, nullptr);
            renderPass->setVertexBuffer(0, drawItem.vertexBuffer, 0, drawItem.vertexBufferSize);
            renderPass->setIndexBuffer(drawItem.indexBuffer, wgpu::IndexFormat::Uint16, 0, drawItem.indexBufferSize);
            renderPass->drawIndexed(drawItem.indexCount, 1, 0, 0, 0);
        }
    }
    renderPass->end();
}

const wgpu::raii::BindGroupLayout& PBRPass::GetMaterialBindGroupLayout() const {
    return m_materialBindGroupLayout;
}

wgpu::RenderPipeline PBRPass::SelectPipeline(const bool doubleSided) const {
    return doubleSided
               ? (m_pipelineDoubleSided ? *m_pipelineDoubleSided : nullptr)
               : (m_pipelineSingleSided ? *m_pipelineSingleSided : nullptr);
}
