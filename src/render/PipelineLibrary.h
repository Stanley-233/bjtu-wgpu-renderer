#ifndef BJTU_WGPU_RENDERER_PIPELINELIBRARY_H
#define BJTU_WGPU_RENDERER_PIPELINELIBRARY_H

#include "webgpu-raii.hpp"

class RenderContext;

class PipelineLibrary {
public:
    struct Pipeline2D {
        wgpu::raii::BindGroupLayout bindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    struct Pipeline3D {
        wgpu::raii::BindGroupLayout bindGroupLayout;
        wgpu::raii::PipelineLayout  layout;
        wgpu::raii::RenderPipeline  pipeline;
    };

    static Pipeline2D CreateColor2D(RenderContext& ctx);

    static Pipeline3D CreateColor3D(RenderContext& ctx);

    static Pipeline3D CreateColor3DWireframe(RenderContext& ctx);

    static Pipeline3D CreateColor3DWireframeDepthPrepass(RenderContext& ctx);
};

#endif // BJTU_WGPU_RENDERER_PIPELINELIBRARY_H
