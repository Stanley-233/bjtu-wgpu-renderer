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

struct PassContext {
    std::optional<RenderCamera>               camera{};
    std::optional<DirectionalShadowPassData> directionalShadow{};
    RenderLightSet                            lights{};
    std::span<const PreparedDrawItem>         drawItems{};
    LegacyGuiRenderer*                        guiRenderer = nullptr;
    wgpu::Queue*                              queue = nullptr;
    wgpu::TextureView                         fallbackShadowMapView = nullptr;
    wgpu::Sampler                             fallbackShadowSampler = nullptr;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void Render(RenderContext& ctx, RenderFrame& frame, const PassContext& context) = 0;
};

#endif // BJTU_WGPU_RENDERER_IRENDERPASS_H
