#ifndef BJTU_WGPU_RENDERER_LOGICSCENE_H
#define BJTU_WGPU_RENDERER_LOGICSCENE_H

#include "render/Renderer.h"
#include "scene/IScene.h"
#include "scene/World.h"

class LogicScene final : public IScene, public ICameraMoveInputSink {
public:
    void Initialize(RenderContext& ctx) override;

    void Update(float dt) override;

    void Render(RenderContext& ctx, LegacyGuiRenderer& guiRenderer) override;

    [[nodiscard]] const char* Name() const override;

    void RegisterInputHandlers(InputEventBus& eventBus) override;

    void UnregisterInputHandlers(InputEventBus& eventBus) override;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) override;

private:
    RenderScene BuildRenderScene(const RenderContext& ctx) const;

    static constexpr float kCameraMoveSpeed = 2.5f;

    World    m_world{};
    Renderer m_renderer{};
    float    m_moveForward = 0.0f;
    float    m_moveRight   = 0.0f;
    float    m_moveUp      = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_LOGICSCENE_H
