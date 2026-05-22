#ifndef BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
#define BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H

#include "webgpu-raii.hpp"

class RenderContext;

class Scene3DPipelineFactory {
public:
    struct ForwardPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::BindGroupLayout materialBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct PbrPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::BindGroupLayout materialBindGroupLayout;
        wgpu::raii::BindGroupLayout debugBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct DepthPrepassPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct SceneNormalPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct SsaoPipeline {
        wgpu::raii::BindGroupLayout ssaoBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct CompositePipeline {
        wgpu::raii::BindGroupLayout sceneColorBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct ToneMapPipeline {
        wgpu::raii::BindGroupLayout sceneColorBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct SkyboxPipeline {
        wgpu::raii::BindGroupLayout skyboxBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct EquirectToCubemapComputePipeline {
        wgpu::raii::BindGroupLayout bindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::ComputePipeline pipeline;
    };

    struct ShadowPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static ForwardPipeline CreateUnlitForwardPipeline(
        RenderContext& renderCtx,
        wgpu::TextureFormat colorTargetFormat,
        wgpu::CullMode cullMode = wgpu::CullMode::Back);

    static ForwardPipeline CreateLambertForwardPipeline(
        RenderContext& renderCtx,
        wgpu::TextureFormat colorTargetFormat,
        wgpu::CullMode cullMode = wgpu::CullMode::Back);

    static PbrPipeline CreatePbrForwardPipeline(
        RenderContext& renderCtx,
        wgpu::TextureFormat colorTargetFormat,
        wgpu::CullMode cullMode = wgpu::CullMode::Back);

    static DepthPrepassPipeline CreateDepthPrepassPipeline(RenderContext& renderCtx);

    static SceneNormalPipeline CreateSceneNormalPipeline(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    static SsaoPipeline CreateSsaoPipeline(RenderContext& renderCtx);

    static CompositePipeline CreateCompositePipeline(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    static ToneMapPipeline CreateToneMapPipeline(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    static SkyboxPipeline CreateSkyboxPipeline(RenderContext& renderCtx, wgpu::TextureFormat colorTargetFormat);

    static EquirectToCubemapComputePipeline CreateEquirectToCubemapComputePipeline(RenderContext& renderCtx);

    static ShadowPipeline CreateDirectionalShadowPipeline(RenderContext& renderCtx);
};

#endif // BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
