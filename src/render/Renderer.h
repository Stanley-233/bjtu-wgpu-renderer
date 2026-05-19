#ifndef BJTU_WGPU_RENDERER_RENDERER_H
#define BJTU_WGPU_RENDERER_RENDERER_H

#include <vector>

#include "frame/RenderFrame.h"
#include "gpu/GpuResourceCache.h"
#include "passes/ForwardPass.h"
#include "passes/GuiPass.h"
#include "passes/PreparedDrawItem.h"
#include "passes/ShadowPass.h"
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
    static constexpr uint32_t kDirectionalShadowMapResolution = 2048;

    struct DrawItemResources {
        wgpu::raii::BindGroup materialBindGroup;
    };

    [[nodiscard]] RenderFrame BeginRenderFrame(RenderContext& ctx);

    void EnsureDepthResources(RenderContext& ctx, int width, int height);

    void EnsureDirectionalShadowResources(RenderContext& ctx, uint32_t width, uint32_t height);

    void EnsureFallbackShadowResources(RenderContext& ctx);

    void BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene);

    GpuResourceCache               m_resourceCache;
    ShadowPass                     m_shadowPass;
    ForwardPass                    m_forwardPass;
    GuiPass                        m_guiPass;
    wgpu::raii::Texture            m_depthTexture;
    wgpu::raii::TextureView        m_depthView;
    wgpu::raii::Texture            m_directionalShadowTexture;
    wgpu::raii::TextureView        m_directionalShadowView;
    wgpu::raii::Sampler            m_directionalShadowSampler;
    wgpu::raii::Texture            m_fallbackShadowTexture;
    wgpu::raii::TextureView        m_fallbackShadowView;
    wgpu::raii::Sampler            m_fallbackShadowSampler;
    int                            m_depthWidth  = 0;
    int                            m_depthHeight = 0;
    uint32_t                       m_directionalShadowWidth  = 0;
    uint32_t                       m_directionalShadowHeight = 0;
    std::vector<DrawItemResources> m_drawItemResources;
    std::vector<PreparedDrawItem> m_preparedDrawItems;
    wgpu::Color                    m_clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERER_H
