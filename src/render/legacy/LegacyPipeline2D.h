#ifndef BJTU_WGPU_RENDERER_LEGACYPIPELINE2D_H
#define BJTU_WGPU_RENDERER_LEGACYPIPELINE2D_H

#include "webgpu-raii.hpp"

class RenderContext;

class LegacyPipeline2D {
public:
    struct Pipeline {
        wgpu::raii::BindGroupLayout bindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static Pipeline Create(RenderContext& ctx);
};

#endif // BJTU_WGPU_RENDERER_LEGACYPIPELINE2D_H
