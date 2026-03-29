#include "InputManager.h"

#include <GLFW/glfw3.h>

namespace {
constexpr float kTranslateStep = 0.05f;
}

InputManager::InputManager() {
    InitializeDefaultBindings();
}

void InputManager::SetDebugEnabled(const bool enabled) {
    m_eventLogger.SetEnabled(enabled);
    if (enabled && !m_debugSinkConnected) {
        m_dispatcher.sink<TransformActionEvent>().connect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_debugSinkConnected = true;
    } else if (!enabled && m_debugSinkConnected) {
        m_dispatcher.sink<TransformActionEvent>().disconnect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_debugSinkConnected = false;
    }
}

void InputManager::EmitKeyEvent(const int key, const int action, const int mods) {
    m_eventLogger.LogRawKeyEvent(key, action, mods);

    if (action != GLFW_PRESS) {
        return;
    }

    const auto it = m_keyBindings.find(key);
    if (it == m_keyBindings.end()) {
        m_eventLogger.LogNoBinding(key);
        return;
    }

    const auto& binding = it->second;
    m_dispatcher.trigger<TransformActionEvent>({binding.action, binding.amountX, binding.amountY});
}

void InputManager::SubscribeTransformActions(IScene& scene) {
    m_dispatcher.sink<TransformActionEvent>().connect<&IScene::OnTransformInputEvent>(scene);
}

void InputManager::UnsubscribeTransformActions(IScene& scene) {
    m_dispatcher.sink<TransformActionEvent>().disconnect<&IScene::OnTransformInputEvent>(scene);
}

void InputManager::InitializeDefaultBindings() {
    m_keyBindings.clear();
    // 平移变换
    m_keyBindings.emplace(GLFW_KEY_W, TransformBinding{ETransformAction::Translate, 0.0f, kTranslateStep});
    m_keyBindings.emplace(GLFW_KEY_S, TransformBinding{ETransformAction::Translate, 0.0f, -kTranslateStep});
    m_keyBindings.emplace(GLFW_KEY_A, TransformBinding{ETransformAction::Translate, -kTranslateStep, 0.0f});
    m_keyBindings.emplace(GLFW_KEY_D, TransformBinding{ETransformAction::Translate, kTranslateStep, 0.0f});
    // TODO: 旋转、缩放等按键绑定
}
