#ifndef BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H
#define BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H

#include <cstdint>

#include <glm/mat4x4.hpp>

#include "asset/types/MaterialAsset.h"
#include "render/scene/RenderUniformData.h"
#include "webgpu-raii.hpp"

struct PreparedDrawItem {
    EMaterialShadingModel shadingModel{EMaterialShadingModel::Unlit};
    glm::mat4             model{1.0f};
    ObjectUniformData     objectUniformData{};
    wgpu::Buffer          vertexBuffer       = nullptr;
    wgpu::Buffer          indexBuffer        = nullptr;
    wgpu::BindGroup       materialBindGroup  = nullptr;
    uint64_t              vertexBufferSize   = 0;
    uint64_t              indexBufferSize    = 0;
    uint32_t              indexCount         = 0;
};

#endif // BJTU_WGPU_RENDERER_PREPAREDDRAWITEM_H
