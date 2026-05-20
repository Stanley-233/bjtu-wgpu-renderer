#include "SceneManager.h"

#include <stdexcept>

#include "input/InputEventBus.h"
#include "render/RenderContext.h"

void SceneManager::RegisterScene(const ESceneType type, SceneFactory factory) {
    m_sceneFactories[type] = std::move(factory);
}

void SceneManager::SetInputEventBus(InputEventBus& eventBus) {
    if (m_activeScene && m_inputEventBus != nullptr) {
        m_activeScene->UnregisterInputHandlers(*m_inputEventBus);
    }
    m_inputEventBus = &eventBus;
    if (m_activeScene) {
        m_activeScene->RegisterInputHandlers(*m_inputEventBus);
    }
}

bool SceneManager::SetActiveScene(const ESceneType type, RenderContext& renderCtx) {
    const auto it = m_sceneFactories.find(type);
    if (it == m_sceneFactories.end()) {
        throw std::runtime_error("Scene was not registered.");
    }

    std::unique_ptr<IScene> candidate = it->second();
    if (!candidate) {
        throw std::runtime_error("Scene factory returned a null scene.");
    }
    if (!candidate->Initialize(renderCtx)) {
        return false;
    }

    UnloadActiveScene();

    m_activeSceneType = type;
    m_activeScene = std::move(candidate);
    if (m_inputEventBus != nullptr) {
        m_activeScene->RegisterInputHandlers(*m_inputEventBus);
    }
    return true;
}

void SceneManager::Shutdown() {
    UnloadActiveScene();
}

bool SceneManager::HasActiveScene() const {
    return static_cast<bool>(m_activeScene);
}

IScene& SceneManager::ActiveScene() const {
    if (!m_activeScene) {
        throw std::runtime_error("No active scene.");
    }
    return *m_activeScene;
}

void SceneManager::UpdateActive(float dt) const {
    if (m_activeScene) {
        m_activeScene->Update(dt);
    }
}

void SceneManager::RenderActive(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) const {
    if (m_activeScene) {
        m_activeScene->Render(renderCtx, guiRenderer);
    }
}

void SceneManager::UnloadActiveScene() {
    if (m_activeScene && m_inputEventBus != nullptr) {
        m_activeScene->UnregisterInputHandlers(*m_inputEventBus);
    }
    m_activeScene.reset();
    m_activeSceneType.reset();
}
