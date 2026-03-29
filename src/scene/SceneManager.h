#ifndef BJTU_WGPU_RENDERER_SCENEMANAGER_H
#define BJTU_WGPU_RENDERER_SCENEMANAGER_H

#include <memory>
#include <unordered_map>

#include "Scene.h"

class RenderContext;

class SceneManager {
public:
    void RegisterScene(ESceneType type, std::unique_ptr<IScene> scene);

    void InitializeAll(RenderContext& ctx);

    void SetActiveScene(ESceneType type);

    IScene& ActiveScene() const;

    void UpdateActive(float dt);

    void RenderActive(RenderContext& ctx);

private:
    std::unordered_map<ESceneType, std::unique_ptr<IScene> > m_scenes;
    IScene*                                                 m_activeScene = nullptr;
};

#endif // BJTU_WGPU_RENDERER_SCENEMANAGER_H