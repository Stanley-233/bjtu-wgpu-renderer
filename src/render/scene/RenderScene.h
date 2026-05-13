#ifndef BJTU_WGPU_RENDERER_RENDERSCENE_H
#define BJTU_WGPU_RENDERER_RENDERSCENE_H

#include <optional>
#include <vector>

#include "RenderCamera.h"
#include "RenderObject.h"

struct RenderScene {
    std::optional<RenderCamera> camera{};
    std::vector<RenderObject>   objects{};
};

#endif // BJTU_WGPU_RENDERER_RENDERSCENE_H
