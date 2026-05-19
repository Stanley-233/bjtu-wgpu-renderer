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

struct DirectionalShadowPassData {
    DirectionalShadowUniformData uniformData{};
    wgpu::TextureView            shadowMapView = nullptr;
    wgpu::Sampler                shadowSampler = nullptr;
};

// 这里不用 RAII 包装，是因为 texture/view 的 owner 是 Renderer。
// Queue 的 owner 是 RenderContext。
// 这里表达的是一帧内借用，不是所有权转移。
struct PassContext {
    std::optional<RenderCamera>               camera{};
    std::optional<DirectionalShadowPassData>  directionalShadow{};
    RenderLightSet                            lights{};
    std::span<const PreparedDrawItem>         drawItems{};
    LegacyGuiRenderer*                        guiRenderer = nullptr;
    wgpu::Queue*                              queue = nullptr;
    wgpu::TextureView                         fallbackShadowMapView = nullptr;
    wgpu::Sampler                             fallbackShadowSampler = nullptr;
    wgpu::TextureView                         sceneDepthView = nullptr;
    wgpu::TextureView                         sceneAoView = nullptr;
    wgpu::TextureView                         sceneColorView = nullptr;
    wgpu::TextureView                         sceneNormalView = nullptr;
    int                                       viewportWidth = 0;
    int                                       viewportHeight = 0;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) = 0;
};

#endif // BJTU_WGPU_RENDERER_IRENDERPASS_H
