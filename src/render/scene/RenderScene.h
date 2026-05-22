#ifndef BJTU_WGPU_RENDERER_RENDERSCENE_H
#define BJTU_WGPU_RENDERER_RENDERSCENE_H

#include <filesystem>
#include <optional>
#include <vector>

#include "asset/AssetId.h"
#include "asset/AssetServer.h"
#include "asset/types/HdrImageAsset.h"
#include "RenderCamera.h"
#include "RenderLightSet.h"
#include "RenderObject.h"
#include "RenderUniformData.h"

struct DirectionalShadowSceneData {
    DirectionalShadowUniformData uniformData{};
};

struct SkyboxSceneData {
    AssetId<HdrImageAsset> hdrImage{};
    uint32_t               faceSize = 512;
};

struct RenderScene {
    std::optional<RenderCamera>              camera{};
    std::optional<DirectionalShadowSceneData> directionalShadow{};
    std::optional<SkyboxSceneData>           skybox{};
    RenderLightSet                           lights{};
    EPbrDebugView                            pbrDebugView = EPbrDebugView::Off;
    const AssetServer*                       assetServer = nullptr;
    std::vector<RenderObject>                objects{};
};

#endif // BJTU_WGPU_RENDERER_RENDERSCENE_H
