#ifndef BJTU_WGPU_RENDERER_RENDERFRAME_H
#define BJTU_WGPU_RENDERER_RENDERFRAME_H

#include "webgpu-raii.hpp"

struct RenderFrame {
    wgpu::raii::CommandEncoder encoder{};
    wgpu::raii::TextureView    surfaceView{};
    wgpu::TextureView          depthView = nullptr;
    int                        drawableWidth = 0;
    int                        drawableHeight = 0;
    wgpu::Color                clearColor{0.08, 0.09, 0.12, 1.0};
};

#endif // BJTU_WGPU_RENDERER_RENDERFRAME_H
