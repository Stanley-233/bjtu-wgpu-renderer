#ifndef BJTU_WGPU_RENDERER_SURFACEFRAME_H
#define BJTU_WGPU_RENDERER_SURFACEFRAME_H

#include "webgpu-raii.hpp"

struct SurfaceFrame {
    wgpu::SurfaceTexture   surfaceTexture{};
    wgpu::raii::TextureView view{};
    int                    surfaceWidth = 0;
    int                    surfaceHeight = 0;
};

#endif // BJTU_WGPU_RENDERER_SURFACEFRAME_H
