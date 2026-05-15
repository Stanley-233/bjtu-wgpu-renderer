#ifndef BJTU_WGPU_RENDERER_RENDERSCENE_H
#define BJTU_WGPU_RENDERER_RENDERSCENE_H

#include <optional>
#include <vector>

#include "asset/AssetServer.h"
#include "RenderCamera.h"
#include "RenderObject.h"

struct RenderScene {
    std::optional<RenderCamera> camera{};
    const AssetServer*          assetServer = nullptr;
    std::vector<RenderObject>   objects{};
};

#endif // BJTU_WGPU_RENDERER_RENDERSCENE_H
