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
        wgpu::raii::BindGroupLayout depthBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct CompositePipeline {
        wgpu::raii::BindGroupLayout sceneColorBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static ForwardPipeline CreateUnlitForwardPipeline(RenderContext& ctx, wgpu::TextureFormat colorTargetFormat);

    static ForwardPipeline CreateLambertForwardPipeline(RenderContext& ctx, wgpu::TextureFormat colorTargetFormat);

    static DepthPrepassPipeline CreateDepthPrepassPipeline(RenderContext& ctx);

    static SceneNormalPipeline CreateSceneNormalPipeline(RenderContext& ctx, wgpu::TextureFormat colorTargetFormat);

    static SsaoPipeline CreateSsaoPipeline(RenderContext& ctx);

    static CompositePipeline CreateCompositePipeline(RenderContext& ctx, wgpu::TextureFormat colorTargetFormat);
};

#endif // BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
