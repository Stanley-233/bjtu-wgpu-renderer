#ifndef BJTU_WGPU_RENDERER_LOGICSCENE_H
#define BJTU_WGPU_RENDERER_LOGICSCENE_H

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

#include "asset/AssetServer.h"
#include "asset/types/HdrImageAsset.h"
#include "asset/types/MaterialAsset.h"
#include "render/Renderer.h"
#include "render/legacy/LegacyGuiRenderer.h"
#include "scene/IScene.h"
#include "scene/World.h"
#include "scene/camera/CameraController.h"

struct CameraComponent;
struct DirectionalLightComponent;
struct ModelAsset;

class LogicScene : public IScene, public ICameraMoveInputSink, public ICameraLookInputSink {
public:
    struct ModelSpawnOptions {
        std::optional<EMaterialShadingModel> shadingModelOverride{};
        bool                                 skipMaskedMaterials = false;
    };

    LogicScene();

    bool Initialize(RenderContext& renderCtx) override;

    void Update(float dt) override;

    void Render(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) override;

    void SetSsaoEnabled(bool enabled);

    void SetToneMapSettings(const ToneMapSettings& settings);

    void SetLitShadingModelOverride(EMaterialShadingModel shadingModel);

    void SetPbrDebugView(EPbrDebugView debugView);

    // 获取平行光数据（用于 GUI）
    [[nodiscard]] DirectionalLightGuiData GetDirectionalLightData() const;

    // 设置平行光数据（用于 GUI）
    void SetDirectionalLightData(const DirectionalLightGuiData& data);

    // 获取摄像机数据（用于 GUI）
    [[nodiscard]] CameraGuiData GetCameraData() const;

    [[nodiscard]] virtual const char* Name() const override = 0;

    void RegisterInputHandlers(InputEventBus& eventBus) override;

    void UnregisterInputHandlers(InputEventBus& eventBus) override;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) override;

    void OnCameraLookInputEvent(const CameraLookInputEvent& event) override;

protected:
    [[nodiscard]] virtual bool BuildSceneContent() = 0;

    virtual void ConfigureInitialCamera(CameraComponent& camera) = 0;

    virtual void ConfigureInitialDirectionalLight(DirectionalLightComponent& light) = 0;

    [[nodiscard]] World& GetWorld();

    [[nodiscard]] const World& GetWorld() const;

    [[nodiscard]] AssetServer& GetAssetServer();

    [[nodiscard]] const AssetServer& GetAssetServer() const;

    [[nodiscard]] Entity LoadModelRoot(const std::filesystem::path& path, std::string_view namePrefix);

    [[nodiscard]] Entity LoadModelRoot(
        const std::filesystem::path& path,
        std::string_view             namePrefix,
        ModelSpawnOptions            options);

    [[nodiscard]] Entity SpawnModelEntities(const ModelAsset& model, std::string_view namePrefix);

    [[nodiscard]] Entity SpawnModelEntities(
        const ModelAsset&  model,
        std::string_view   namePrefix,
        ModelSpawnOptions  options);

    void SetStaticMeshShadingModelOverride(Entity entity, std::optional<EMaterialShadingModel> shadingModel);

    void SetStaticMeshShadingModelOverrideRecursive(Entity root, std::optional<EMaterialShadingModel> shadingModel);

private:
    RenderScene BuildRenderScene(const RenderContext& renderCtx) const;

    RenderLightSet BuildRenderLightSet() const;

    World                             m_world{};
    AssetServer                       m_assetServer{};
    Renderer                          m_renderer{};
    std::unique_ptr<CameraController> m_cameraController{};
    AssetId<HdrImageAsset>            m_defaultSkyboxHdrImage{};
    EPbrDebugView                     m_pbrDebugView = EPbrDebugView::Off;
};

#endif // BJTU_WGPU_RENDERER_LOGICSCENE_H
