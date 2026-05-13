#ifndef BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
#define BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H

#include "webgpu-raii.hpp"

class RenderContext;

class Scene3DPipelineFactory {
public:
    struct Pipeline {
        wgpu::raii::BindGroupLayout bindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static Pipeline CreateForwardPipeline(RenderContext& ctx);

    static Pipeline CreateWireframePipeline(RenderContext& ctx);

    static Pipeline CreateWireframeDepthPrepassPipeline(RenderContext& ctx);
};

#endif // BJTU_WGPU_RENDERER_SCENE3DPIPELINEFACTORY_H
