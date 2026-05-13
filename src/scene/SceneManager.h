#ifndef BJTU_WGPU_RENDERER_SCENEMANAGER_H
#define BJTU_WGPU_RENDERER_SCENEMANAGER_H

#include <memory>
#include <unordered_map>

#include "IScene.h"

class RenderContext;
class GuiRenderer;
class InputEventBus;

class SceneManager {
public:
    void RegisterScene(ESceneType type, std::unique_ptr<IScene> scene);

    void InitializeAll(RenderContext& ctx);

    void SetInputEventBus(InputEventBus& eventBus);

    void SetActiveScene(ESceneType type);

    IScene& ActiveScene() const;

    void UpdateActive(float dt) const;

    void RenderActive(RenderContext& ctx, GuiRenderer& guiRenderer) const;

private:
    InputEventBus*                                      m_inputEventBus = nullptr;
    std::unordered_map<ESceneType, std::unique_ptr<IScene> > m_scenes;
    IScene*                                                 m_activeScene = nullptr;
};

#endif // BJTU_WGPU_RENDERER_SCENEMANAGER_H
