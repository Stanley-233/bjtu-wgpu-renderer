#ifndef BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H
#define BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H

#include <cstdint>

#include <glm/mat4x4.hpp>

#include "scene/legacy/Object3D.h"
#include "webgpu-raii.hpp"

struct PreparedDrawItem {
    glm::mat4             model{1.0f};
    Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
    wgpu::Buffer          vertexBuffer = nullptr;
    wgpu::Buffer          indexBuffer = nullptr;
    wgpu::Buffer          wireframeDepthIndexBuffer = nullptr;
    wgpu::Buffer          uniformBuffer = nullptr;
    wgpu::BindGroup       bindGroup = nullptr;
    uint64_t              vertexBufferSize = 0;
    uint64_t              indexBufferSize = 0;
    uint64_t              wireframeDepthIndexBufferSize = 0;
    uint32_t              indexCount = 0;
    uint32_t              wireframeDepthIndexCount = 0;
};

#endif // BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H
