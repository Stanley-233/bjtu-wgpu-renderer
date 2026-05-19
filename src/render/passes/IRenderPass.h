#ifndef BJTU_WGPU_RENDERER_IRENDERPASS_H
#define BJTU_WGPU_RENDERER_IRENDERPASS_H

#include <optional>
#include <span>

#include "PreparedDrawItem.h"
#include "render/scene/RenderCamera.h"
#include "render/scene/RenderLightSet.h"
#include "webgpu-raii.hpp"

class LegacyGuiRenderer;
class RenderContext;
struct RenderFrame;

struct PassContext {
    std::optional<RenderCamera>       camera{};
    RenderLightSet                    lights{};
    std::span<const PreparedDrawItem> drawItems{};
    LegacyGuiRenderer*               guiRenderer = nullptr;
    wgpu::Queue*                     queue = nullptr;
    wgpu::TextureView                sceneDepthView = nullptr;
    wgpu::TextureView                sceneAoView = nullptr;
    int                              viewportWidth = 0;
    int                              viewportHeight = 0;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) = 0;
};

#endif // BJTU_WGPU_RENDERER_IRENDERPASS_H
