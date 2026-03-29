#include "SceneManager.h"

#include <stdexcept>

#include "../render/RenderContext.h"

void SceneManager::RegisterScene(ESceneType type, std::unique_ptr<IScene> scene) {
    m_scenes[type] = std::move(scene);
}

void SceneManager::InitializeAll(RenderContext& ctx) {
    for (auto& [type, scene] : m_scenes) {
        (void)type;
        scene->Initialize(ctx);
    }
}

void SceneManager::SetActiveScene(ESceneType type) {
    const auto it = m_scenes.find(type);
    if (it == m_scenes.end()) {
        throw std::runtime_error("Scene was not registered.");
    }
    m_activeScene = it->second.get();
}

IScene& SceneManager::ActiveScene() const {
    if (m_activeScene == nullptr) {
        throw std::runtime_error("No active scene.");
    }
    return *m_activeScene;
}

void SceneManager::UpdateActive(float dt) {
    ActiveScene().Update(dt);
}

void SceneManager::RenderActive(RenderContext& ctx) {
    ActiveScene().Render(ctx);
}