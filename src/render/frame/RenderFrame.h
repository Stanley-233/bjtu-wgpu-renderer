#ifndef BJTU_WGPU_RENDERER_RENDERFRAME_H
#define BJTU_WGPU_RENDERER_RENDERFRAME_H

#include "SurfaceFrame.h"
#include "webgpu-raii.hpp"

struct RenderFrame {
    SurfaceFrame                surfaceFrame{};
    wgpu::raii::CommandEncoder  encoder{};
    wgpu::TextureView           sceneDepthView = nullptr;
    wgpu::TextureView           sceneAoView    = nullptr;
    wgpu::TextureView           sceneColorView = nullptr;
    wgpu::TextureView           sceneNormalView = nullptr;
    wgpu::Color                 clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERFRAME_H
