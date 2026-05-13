#ifndef BJTU_WGPU_RENDERER_RENDERER_H
#define BJTU_WGPU_RENDERER_RENDERER_H

#include <vector>

#include "frame/RenderFrame.h"
#include "gpu/GpuResourceCache.h"
#include "passes/ForwardPass.h"
#include "passes/GuiPass.h"
#include "passes/PreparedDrawItem.h"
#include "passes/WireframePass.h"
#include "render/scene/RenderScene.h"
#include "webgpu-raii.hpp"

class GuiRenderer;
class RenderContext;

class Renderer {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderContext& ctx, const RenderScene& scene, GuiRenderer& guiRenderer);

    void SetClearColor(double r, double g, double b, double a);

private:
    struct ObjectResources {
        wgpu::raii::Buffer    uniformBuffer;
        wgpu::raii::BindGroup bindGroup;
    };

    [[nodiscard]] RenderFrame BeginRenderFrame(RenderContext& ctx);

    void EnsureDepthResources(RenderContext& ctx, int width, int height);

    void EnsureObjectResources(RenderContext& ctx, std::size_t objectCount);

    void BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene);

    GpuResourceCache              m_resourceCache;
    ForwardPass                   m_forwardPass;
    WireframePass                 m_wireframePass;
    GuiPass                       m_guiPass;
    wgpu::raii::Texture           m_depthTexture;
    wgpu::raii::TextureView       m_depthView;
    int                           m_depthWidth  = 0;
    int                           m_depthHeight = 0;
    std::vector<ObjectResources>  m_objectResources;
    std::vector<PreparedDrawItem> m_preparedDrawItems;
    wgpu::Color                   m_clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER_H