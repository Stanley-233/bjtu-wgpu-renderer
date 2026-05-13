#include "GuiPass.h"

#include "render/GuiRenderer.h"
#include "render/frame/RenderFrame.h"

void GuiPass::Render(RenderFrame& frame, const PassContext& context) {
    if (!frame.surfaceView || !frame.encoder || context.guiRenderer == nullptr) {
        return;
    }

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = *frame.surfaceView;
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
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    wgpu::raii::RenderPassEncoder renderPass = frame.encoder->beginRenderPass(renderPassDesc);
    context.guiRenderer->Render(renderPass);
    renderPass->end();
}
