#ifndef BJTU_WGPU_RENDERER_SCENEMANAGER_H
#define BJTU_WGPU_RENDERER_SCENEMANAGER_H

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include "IScene.h"

class RenderContext;
class LegacyGuiRenderer;
class InputEventBus;

class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<IScene>()>;

    void RegisterScene(ESceneType type, SceneFactory factory);

    void SetInputEventBus(InputEventBus& eventBus);

    bool SetActiveScene(ESceneType type, RenderContext& renderCtx);

    void Shutdown();

    [[nodiscard]] bool HasActiveScene() const;

    IScene& ActiveScene() const;

    void UpdateActive(float dt) const;

    void RenderActive(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) const;

private:
    void UnloadActiveScene();

    InputEventBus*                                  m_inputEventBus = nullptr;
    std::unordered_map<ESceneType, SceneFactory>    m_sceneFactories;
    std::optional<ESceneType>                       m_activeSceneType;
    std::unique_ptr<IScene>                         m_activeScene;
};

#endif // BJTU_WGPU_RENDERER_SCENEMANAGER_H
