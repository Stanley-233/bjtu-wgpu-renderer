#ifndef BJTU_WGPU_RENDERER_LOGICSCENE_H
#define BJTU_WGPU_RENDERER_LOGICSCENE_H

#include <memory>

#include "render/Renderer.h"
#include "scene/IScene.h"
#include "scene/World.h"
#include "scene/camera/CameraController.h"

class LogicScene final : public IScene, public ICameraMoveInputSink {
public:
    LogicScene();

    void Initialize(RenderContext& ctx) override;

    void Update(float dt) override;

    void Render(RenderContext& ctx, LegacyGuiRenderer& guiRenderer) override;

    [[nodiscard]] const char* Name() const override;

    void RegisterInputHandlers(InputEventBus& eventBus) override;

    void UnregisterInputHandlers(InputEventBus& eventBus) override;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) override;

private:
    RenderScene BuildRenderScene(const RenderContext& ctx) const;

    World                             m_world{};
    Renderer                          m_renderer{};
    std::unique_ptr<CameraController> m_cameraController{};
};

#endif // BJTU_WGPU_RENDERER_LOGICSCENE_H
