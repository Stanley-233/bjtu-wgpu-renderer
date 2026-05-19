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

    struct SsaoPipeline {
        wgpu::raii::BindGroupLayout depthBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static ForwardPipeline CreateUnlitForwardPipeline(RenderContext& ctx);

    static ForwardPipeline CreateLambertForwardPipeline(RenderContext& ctx);

    static DepthPrepassPipeline CreateDepthPrepassPipeline(RenderContext& ctx);

    static SsaoPipeline CreateSsaoPipeline(RenderContext& ctx);
};

#endif // BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
