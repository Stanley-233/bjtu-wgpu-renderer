#ifndef BJTU_WGPU_RENDERER_RENDERER_H
#define BJTU_WGPU_RENDERER_RENDERER_H

#include <vector>

#include "frame/RenderFrame.h"
#include "gpu/GpuResourceCache.h"
#include "passes/DepthPrepass.h"
#include "passes/ForwardOpaquePass.h"
#include "passes/GuiPass.h"
#include "passes/PBRPass.h"
#include "passes/PreparedDrawItem.h"
#include "passes/SceneNormalPass.h"
#include "passes/SSAOPass.h"
#include "passes/SkyboxPass.h"
#include "passes/ShadowPass.h"
#include "passes/ToneMapPass.h"
#include "render/scene/RenderScene.h"
#include "webgpu-raii.hpp"

class LegacyGuiRenderer;
class RenderContext;

class Renderer {
public:
    static constexpr uint32_t kSkyboxCubemapFaceSize = 512;
    inline static const wgpu::TextureFormat kHdrSceneColorFormat = wgpu::TextureFormat::RGBA16Float;

    void Initialize(RenderContext& renderCtx);

    void Render(RenderContext& renderCtx, const RenderScene& scene, LegacyGuiRenderer& guiRenderer);

    void SetSsaoEnabled(bool enabled);

    void SetClearColor(double r, double g, double b, double a);

    void PrepareSkybox(RenderContext& renderCtx, const std::filesystem::path& hdrPath, uint32_t faceSize);

private:
    static constexpr uint32_t kDirectionalShadowMapResolution = 2048;

    struct DrawItemResources {
        wgpu::raii::BindGroup forwardMaterialBindGroup;
        wgpu::raii::BindGroup pbrMaterialBindGroup;
    };

    [[nodiscard]] RenderFrame BeginRenderFrame(RenderContext& renderCtx);

    void EnsureFrameResources(RenderContext& renderCtx, int width, int height);

    void EnsureDirectionalShadowResources(RenderContext& renderCtx, uint32_t width, uint32_t height);

    void EnsureFallbackShadowResources(RenderContext& renderCtx);

    void BuildPreparedDrawItems(RenderContext& renderCtx, const RenderScene& scene);

    GpuResourceCache                m_resourceCache;
    ShadowPass                      m_shadowPass;
    DepthPrepass                    m_depthPrepass;
    SceneNormalPass                 m_sceneNormalPass;
    SSAOPass                        m_ssaoPass;
    SkyboxPass                      m_skyboxPass;
    ForwardOpaquePass               m_forwardOpaquePass;
    PBRPass                         m_pbrPass;
    ToneMapPass                     m_toneMapPass;
    GuiPass                         m_guiPass;
    wgpu::raii::Texture             m_sceneDepthTexture;
    wgpu::raii::TextureView         m_sceneDepthView;
    wgpu::raii::Texture             m_sceneAoTexture;
    wgpu::raii::TextureView         m_sceneAoView;
    wgpu::raii::Texture             m_sceneColorTexture;
    wgpu::raii::TextureView         m_sceneColorView;
    wgpu::raii::Texture             m_sceneNormalTexture;
    wgpu::raii::TextureView         m_sceneNormalView;
    wgpu::raii::Texture             m_directionalShadowTexture;
    wgpu::raii::TextureView         m_directionalShadowView;
    wgpu::raii::Sampler             m_directionalShadowSampler;
    wgpu::raii::Texture             m_fallbackShadowTexture;
    wgpu::raii::TextureView         m_fallbackShadowView;
    wgpu::raii::Sampler             m_fallbackShadowSampler;
    int                             m_frameResourceWidth  = 0;
    int                             m_frameResourceHeight = 0;
    uint32_t                        m_directionalShadowWidth  = 0;
    uint32_t                        m_directionalShadowHeight = 0;
    std::vector<DrawItemResources>  m_drawItemResources;
    std::vector<PreparedDrawItem>   m_preparedDrawItems;
    wgpu::Color                     m_clearColor{0.08, 0.09, 0.12, 1.0};
    bool                            m_ssaoEnabled = true;
};

#endif // BJTU_WGPU_RENDERER_RENDERER_H
