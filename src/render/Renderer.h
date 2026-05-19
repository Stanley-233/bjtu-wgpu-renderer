#ifndef BJTU_WGPU_RENDERER_RENDERER_H
#define BJTU_WGPU_RENDERER_RENDERER_H

#include <vector>

#include "frame/RenderFrame.h"
#include "gpu/GpuResourceCache.h"
#include "passes/DepthPrepass.h"
#include "passes/ForwardOpaquePass.h"
#include "passes/GuiPass.h"
#include "passes/PreparedDrawItem.h"
#include "passes/SSAOPass.h"
#include "render/scene/RenderScene.h"
#include "webgpu-raii.hpp"

class LegacyGuiRenderer;
class RenderContext;

class Renderer {
public:
    void Initialize(RenderContext& ctx);

    void Render(RenderContext& ctx, const RenderScene& scene, LegacyGuiRenderer& guiRenderer);

    void SetClearColor(double r, double g, double b, double a);

private:
    struct DrawItemResources {
        wgpu::raii::BindGroup materialBindGroup;
    };

    [[nodiscard]] RenderFrame BeginRenderFrame(RenderContext& ctx);

    void EnsureFrameResources(RenderContext& ctx, int width, int height);

    void BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene);

    GpuResourceCache               m_resourceCache;
    DepthPrepass                   m_depthPrepass;
    SSAOPass                       m_ssaoPass;
    ForwardOpaquePass              m_forwardOpaquePass;
    GuiPass                        m_guiPass;
    wgpu::raii::Texture            m_sceneDepthTexture;
    wgpu::raii::TextureView        m_sceneDepthView;
    wgpu::raii::Texture            m_sceneAoTexture;
    wgpu::raii::TextureView        m_sceneAoView;
    int                            m_frameResourceWidth  = 0;
    int                            m_frameResourceHeight = 0;
    std::vector<DrawItemResources> m_drawItemResources;
    std::vector<PreparedDrawItem> m_preparedDrawItems;
    wgpu::Color                    m_clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER_H
