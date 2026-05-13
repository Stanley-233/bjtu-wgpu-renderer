#ifndef BJTU_WGPU_RENDERER_GPUMESH_H
#define BJTU_WGPU_RENDERER_GPUMESH_H

#include <cstdint>

#include "scene/legacy/Object3D.h"
#include "webgpu-raii.hpp"

struct GpuMesh {
    wgpu::raii::Buffer vertexBuffer;
    wgpu::raii::Buffer indexBuffer;
    wgpu::raii::Buffer wireframeDepthIndexBuffer;
    uint64_t           vertexBufferSize = 0;
    uint64_t           indexBufferSize = 0;
    uint64_t           wireframeDepthIndexBufferSize = 0;
    uint32_t           indexCount = 0;
    uint32_t           wireframeDepthIndexCount = 0;
    uint32_t           sourceVertexCount = 0;
    uint32_t           sourceIndexCount = 0;
    Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
};

#endif // BJTU_WGPU_RENDERER_GPUMESH_H
