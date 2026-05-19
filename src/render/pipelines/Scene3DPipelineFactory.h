#ifndef BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
#define BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H

#include "webgpu-raii.hpp"

class RenderContext;

class Scene3DPipelineFactory {
public:
    struct Pipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::BindGroupLayout materialBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct ShadowPipeline {
        wgpu::raii::BindGroupLayout sceneBindGroupLayout;
        wgpu::raii::BindGroupLayout objectBindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static Pipeline CreateUnlitForwardPipeline(RenderContext& ctx);

    static Pipeline CreateLambertForwardPipeline(RenderContext& ctx);

    static ShadowPipeline CreateDirectionalShadowPipeline(RenderContext& ctx);
};

#endif // BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
