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

// 这里不用 RAII 包装，是因为texture 和 view 的 owner 是 Render
// Queue 的 owner 是 RenderContext
// 这里用来表达借用一帧，不是所有权转移。而且Pass真调用的时候也是用的裸指针
struct PassContext {
    std::optional<RenderCamera>       camera{};
    RenderLightSet                    lights{};
    std::span<const PreparedDrawItem> drawItems{};
    LegacyGuiRenderer*                guiRenderer     = nullptr;
    wgpu::Queue*                      queue           = nullptr;
    wgpu::TextureView                 sceneDepthView  = nullptr;
    wgpu::TextureView                 sceneAoView     = nullptr;
    wgpu::TextureView                 sceneColorView  = nullptr;
    wgpu::TextureView                 sceneNormalView = nullptr;
    int                               viewportWidth   = 0;
    int                               viewportHeight  = 0;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) = 0;
};

#endif // BJTU_WGPU_RENDERER_IRENDERPASS_H
