#include "SceneManager.h"

#include <stdexcept>

#include "input/InputEventBus.h"
#include "render/RenderContext.h"

void SceneManager::RegisterScene(ESceneType type, std::unique_ptr<IScene> scene) {
    m_scenes[type] = std::move(scene);
}

void SceneManager::InitializeAll(RenderContext& ctx) {
    for (auto& [type, scene] : m_scenes) {
        (void)type;
        scene->Initialize(ctx);
    }
}

void SceneManager::SetInputEventBus(InputEventBus& eventBus) {
    if (m_activeScene != nullptr && m_inputEventBus != nullptr) {
        m_activeScene->UnregisterInputHandlers(*m_inputEventBus);
    }
    m_inputEventBus = &eventBus;
    if (m_activeScene != nullptr) {
        m_activeScene->RegisterInputHandlers(*m_inputEventBus);
    }
}

void SceneManager::SetActiveScene(ESceneType type) {
    const auto it = m_scenes.find(type);
    if (it == m_scenes.end()) {
        throw std::runtime_error("Scene was not registered.");
    }
    if (m_activeScene != nullptr && m_inputEventBus != nullptr) {
        m_activeScene->UnregisterInputHandlers(*m_inputEventBus);
    }
    m_activeScene = it->second.get();
    if (m_inputEventBus != nullptr) {
        m_activeScene->RegisterInputHandlers(*m_inputEventBus);
    }
}

IScene& SceneManager::ActiveScene() const {
    if (m_activeScene == nullptr) {
        throw std::runtime_error("No active scene.");
    }
    return *m_activeScene;
}

void SceneManager::UpdateActive(float dt) const {
    ActiveScene().Update(dt);
}

void SceneManager::RenderActive(RenderContext& ctx, GuiRenderer& guiRenderer) const {
    ActiveScene().Render(ctx, guiRenderer);
}
