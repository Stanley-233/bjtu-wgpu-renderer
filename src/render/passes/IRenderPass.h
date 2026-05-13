#ifndef BJTU_WGPU_RENDERER_IRENDERPASS_H
#define BJTU_WGPU_RENDERER_IRENDERPASS_H

#include <optional>
#include <span>

#include "PreparedDrawItem.h"
#include "render/scene/RenderCamera.h"
#include "webgpu-raii.hpp"

class LegacyGuiRenderer;
struct RenderFrame;

struct PassContext {
    std::optional<RenderCamera>       camera{};
    std::span<const PreparedDrawItem> drawItems{};
    LegacyGuiRenderer*              guiRenderer = nullptr;
    wgpu::Queue*                    queue = nullptr;
};

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void Render(RenderFrame& frame, const PassContext& context) = 0;
};

#endif // BJTU_WGPU_RENDERER_IRENDERPASS_H
