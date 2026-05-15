#ifndef BJTU_WGPU_RENDERER_GPUMESH_H
#define BJTU_WGPU_RENDERER_GPUMESH_H

#include <cstdint>

#include "webgpu-raii.hpp"

struct GpuMesh {
    wgpu::raii::Buffer vertexBuffer;
    wgpu::raii::Buffer indexBuffer;
    uint64_t           vertexBufferSize = 0;
    uint64_t           indexBufferSize = 0;
    uint32_t           indexCount = 0;
    uint32_t           sourceVertexCount = 0;
    uint32_t           sourceIndexCount = 0;
};

#endif // BJTU_WGPU_RENDERER_GPUMESH_H
