#include "Scene2D.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "../render/PipelineLibrary.h"
#include "../render/GuiRenderer.h"
#include "../render/RenderContext.h"
#include "../input/InputEventBus.h"
#include "../resource/ResourceManager.h"
#include "../resource/ResourcePaths.h"

using namespace wgpu;

void Scene2D::Initialize(RenderContext& ctx) {
    m_context             = &ctx;
    auto pipeline2D       = PipelineLibrary::CreateColor2D(ctx);
    m_bindGroupLayout     = std::move(pipeline2D.bindGroupLayout);
    m_layout              = std::move(pipeline2D.layout);
    m_pipeline            = std::move(pipeline2D.pipeline);
    InitializeBuffers(ctx);
    InitializeBindGroups(ctx);
}

void Scene2D::Render(RenderContext& ctx, GuiRenderer& guiRenderer) {

    raii::TextureView targetView = ctx.AcquireNextSurfaceView();
    if (!targetView) {
        return;
    }

    UpdateAspectUniform();

    raii::CommandEncoder encoder = ctx.BeginFrame();

    RenderPassDescriptor      renderPassDesc            = {};
    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view                      = *targetView;
    renderPassColorAttachment.resolveTarget             = nullptr;
    renderPassColorAttachment.loadOp                    = LoadOp::Clear;
    renderPassColorAttachment.storeOp                   = StoreOp::Store;
    renderPassColorAttachment.clearValue                = WGPUColor{0.05, 0.05, 0.05, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    renderPassDesc.colorAttachmentCount   = 1;
    renderPassDesc.colorAttachments       = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites        = nullptr;

    raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    renderPass->setPipeline(*m_pipeline);
    renderPass->setVertexBuffer(0, *m_pointBuffer, 0, m_pointBuffer->getSize());
    renderPass->setIndexBuffer(*m_indexBuffer, IndexFormat::Uint16, 0, m_indexBuffer->getSize());
    renderPass->setBindGroup(0, *m_bindGroup, 0, nullptr);
    renderPass->drawIndexed(m_indexCount, 1, 0, 0, 0);
    guiRenderer.Render(renderPass);
    renderPass->end();

    ctx.SubmitAndPresent(encoder);
}

const char* Scene2D::Name() const {
    return "Scene2D";
}

void Scene2D::RegisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<TransformActionEvent>().connect<&Scene2D::OnTransformInputEvent>(*this);
    eventBus.Dispatcher().sink<Transform2DStateEvent>().connect<&Scene2D::OnTransform2DStateEvent>(*this);
}

void Scene2D::UnregisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<TransformActionEvent>().disconnect<&Scene2D::OnTransformInputEvent>(*this);
    eventBus.Dispatcher().sink<Transform2DStateEvent>().disconnect<&Scene2D::OnTransform2DStateEvent>(*this);
}

void Scene2D::Update(const float dt) {
    constexpr float kEpsilon = 1e-6f;
    if (dt <= 0.0f) {
        return;
    }

    if (std::fabs(m_transformState.translateX) > kEpsilon || std::fabs(m_transformState.translateY) > kEpsilon) {
        ApplyTransform(Transform2D::Translation(m_transformState.translateX * dt, m_transformState.translateY * dt));
    }
    if (std::fabs(m_transformState.rotateRate) > kEpsilon) {
        ApplyTransform(Transform2D::Rotation(m_transformState.rotateRate * dt));
    }
    if (std::fabs(m_transformState.scaleXRate) > kEpsilon || std::fabs(m_transformState.scaleYRate) > kEpsilon) {
        const float sx = std::exp(m_transformState.scaleXRate * dt);
        const float sy = std::exp(m_transformState.scaleYRate * dt);
        ApplyTransform(Transform2D::Scale(sx, sy));
    }
    if (std::fabs(m_transformState.shearXRate) > kEpsilon || std::fabs(m_transformState.shearYRate) > kEpsilon) {
        ApplyTransform(Transform2D::Shear(m_transformState.shearXRate * dt, m_transformState.shearYRate * dt));
    }

    // 兼容已存在的一次性积累路径
    if (m_pendingDelta.Matrix() != glm::mat3(1.0f)) {
        ApplyTransform(m_pendingDelta);
        m_pendingDelta.Reset();
    }
}

void Scene2D::OnTransformInputEvent(const TransformActionEvent& event) {
    switch (event.action) {
        case ETransformAction::Translate:
        case ETransformAction::Rotate:
        case ETransformAction::Scale:
        case ETransformAction::Shear:
            // 连续输入由 OnTransform2DStateEvent + Update(dt) 处理
            (void)event.amountX;
            (void)event.amountY;
            break;
        case ETransformAction::ReflectX:
            ApplyTransform(Transform2D::ReflectionX());
            break;
        case ETransformAction::ReflectY:
            ApplyTransform(Transform2D::ReflectionY());
            break;
        case ETransformAction::Reset:
            ResetTransform();
            break;
    }
}

void Scene2D::OnTransform2DStateEvent(const Transform2DStateEvent& event) {
    m_transformState = event;
}

void Scene2D::ApplyTransform(const Transform2D& t) {
    m_transform.Combine(t);
    if (m_context) {
        UploadTransformMatrix(m_transform.Matrix());
    }
}

void Scene2D::ResetTransform() {
    m_transform.Reset();
    m_pendingDelta.Reset();
    if (m_context) {
        UploadTransformMatrix(glm::mat3(1.0f));
    }
}

void Scene2D::UploadTransformMatrix(const glm::mat3& matrix) {
    m_uniformData.transform = Transform2D::ToWgslMat3Uniform(matrix);
    UploadUniformData();
}

void Scene2D::UpdateAspectUniform() {
    if (!m_context) {
        return;
    }

    int drawableWidth  = 0;
    int drawableHeight = 0;
    m_context->GetDrawableSize(drawableWidth, drawableHeight);

    const float width  = static_cast<float>(std::max(1, drawableWidth));
    const float height = static_cast<float>(std::max(1, drawableHeight));
    const float aspect = width / height;
    if (std::fabs(aspect - m_lastAspect) <= std::numeric_limits<float>::epsilon()) {
        return;
    }

    m_lastAspect       = aspect;
    m_uniformData.aspect = aspect;
    UploadUniformData();
}

void Scene2D::UploadUniformData() const {
    if (!m_context || !m_uniformBuffer) {
        return;
    }
    m_context->GetQueue()->writeBuffer(*m_uniformBuffer, 0, &m_uniformData, sizeof(m_uniformData));
}

void Scene2D::InitializeBuffers(RenderContext& ctx) {
    std::vector<float>    pointData;
    std::vector<uint16_t> indexData;

    if (const bool success = ResourceManager::LoadGeometry(ResourcePaths::Resolve("webgpu.txt"), pointData, indexData); !success) {
        std::cerr << "Could not load geometry!" << std::endl;
        std::exit(1);
    }

    m_indexCount = static_cast<uint32_t>(indexData.size());

    BufferDescriptor bufferDesc;
    bufferDesc.size             = pointData.size() * sizeof(float);
    bufferDesc.usage            = BufferUsage::CopyDst | BufferUsage::Vertex;
    bufferDesc.mappedAtCreation = false;
    m_pointBuffer               = ctx.GetDevice()->createBuffer(bufferDesc);
    ctx.GetQueue()->writeBuffer(*m_pointBuffer, 0, pointData.data(), bufferDesc.size);

    bufferDesc.size  = indexData.size() * sizeof(uint16_t);
    bufferDesc.size  = (bufferDesc.size + 3) & ~3;
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
    m_indexBuffer    = ctx.GetDevice()->createBuffer(bufferDesc);
    ctx.GetQueue()->writeBuffer(*m_indexBuffer, 0, indexData.data(), bufferDesc.size);

    bufferDesc.size             = sizeof(m_uniformData);
    bufferDesc.usage            = BufferUsage::CopyDst | BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;
    m_uniformBuffer             = ctx.GetDevice()->createBuffer(bufferDesc);
    // 初始化为单位矩阵 + 当前画布宽高比
    m_uniformData.transform = Transform2D::ToWgslMat3Uniform(glm::mat3(1.0f));
    UpdateAspectUniform();
    UploadUniformData();
}

void Scene2D::InitializeBindGroups(RenderContext& ctx) {
    BindGroupEntry binding{};
    binding.binding = 0;
    binding.buffer  = *m_uniformBuffer;
    binding.offset  = 0;
    binding.size    = sizeof(m_uniformData);

    BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout     = *m_bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries    = &binding;
    m_bindGroup              = ctx.GetDevice()->createBindGroup(bindGroupDesc);
}
