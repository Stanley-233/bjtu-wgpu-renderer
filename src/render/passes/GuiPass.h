#ifndef BJTU_WGPU_RENDERER_GUIPASS_H
#define BJTU_WGPU_RENDERER_GUIPASS_H

#include "IRenderPass.h"

class GuiPass final : public IRenderPass {
public:
    void Render(RenderContext& renderCtx, RenderFrame& frame, const PassContext& passCtx) override;
};

#endif // BJTU_WGPU_RENDERER_GUIPASS_H
