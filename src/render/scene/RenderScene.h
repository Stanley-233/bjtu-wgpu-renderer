#ifndef BJTU_WGPU_RENDERER_RENDERSCENE_H
#define BJTU_WGPU_RENDERER_RENDERSCENE_H

#include <optional>
#include <vector>

#include "asset/AssetServer.h"
#include "RenderCamera.h"
#include "RenderLightSet.h"
#include "RenderObject.h"
#include "RenderUniformData.h"

struct DirectionalShadowSceneData {
    DirectionalShadowUniformData uniformData{};
};

struct RenderScene {
    std::optional<RenderCamera>              camera{};
    std::optional<DirectionalShadowSceneData> directionalShadow{};
    RenderLightSet                           lights{};
    EPbrDebugView                            pbrDebugView = EPbrDebugView::Off;
    const AssetServer*                       assetServer = nullptr;
    std::vector<RenderObject>                objects{};
};

#endif // BJTU_WGPU_RENDERER_RENDERSCENE_H
