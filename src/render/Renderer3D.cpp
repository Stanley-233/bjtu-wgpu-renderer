#include "Renderer3D.h"

#include <utility>

#include "PipelineLibrary.h"
#include "RenderContext.h"
#include "../scene/scene3d/Object3D.h"

void Renderer3D::Initialize(RenderContext& ctx) {
    auto pipeline3D   = PipelineLibrary::CreateColor3D(ctx);
    m_bindGroupLayout = std::move(pipeline3D.bindGroupLayout);
    m_layout          = std::move(pipeline3D.layout);
    m_pipeline        = std::move(pipeline3D.pipeline);
}

void Renderer3D::SyncScene(const std::vector<Object3D>& objects, const Camera& camera) {
    // TODO: 同步场景数据到 GPU（上传顶点/索引、更新模型与相机 uniform、创建/更新 bind group）。
    (void)objects;
    (void)camera;
}

void Renderer3D::RenderFrame(RenderContext& ctx) {
    wgpu::raii::TextureView targetView = ctx.AcquireNextSurfaceView();
    if (!targetView) {
        return;
    }

    wgpu::raii::CommandEncoder encoder = ctx.BeginFrame();

    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view                            = *targetView;
    colorAttachment.resolveTarget                   = nullptr;
    colorAttachment.loadOp                          = wgpu::LoadOp::Clear;
    colorAttachment.storeOp                         = wgpu::StoreOp::Store;
    colorAttachment.clearValue                      = m_clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount       = 1;
    renderPassDesc.colorAttachments           = &colorAttachment;
    renderPassDesc.depthStencilAttachment     = nullptr;
    renderPassDesc.timestampWrites            = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
    // TODO: 在此绑定 3D pipeline、bind group、vertex/index buffer，并发起 drawIndexed
    renderPass->end();

    ctx.SubmitAndPresent(encoder);
}

void Renderer3D::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}

void Renderer3D::ResetGpuResources() {
    m_uniformBuffer  = {};
    m_layout         = {};
    m_bindGroupLayout = {};
    m_bindGroup      = {};
    m_pipeline       = {};
    m_vertexBuffer   = {};
    m_indexBuffer    = {};
    m_indexCount     = 0;
}
