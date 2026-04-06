#include "InputManager.h"

#include <GLFW/glfw3.h>

constexpr float kTranslateStep = 0.05f;
constexpr float kRotateStep = 0.1f; // 旋转步长（弧度）
constexpr float kScaleStep = 1.1f; // 缩放步长
constexpr float kShearStep = 0.1f; // 剪切步长

static bool ShouldSuppressNoBindingLog(const int key) {
    switch (key) {
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_C:
            return true;
        default:
            return false;
    }
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
        if (!ShouldSuppressNoBindingLog(key)) {
            m_eventLogger.LogNoBinding(key);
        }
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
    // 旋转变换
    m_keyBindings.emplace(GLFW_KEY_Q, TransformBinding{ETransformAction::Rotate, kRotateStep, 0.0f});
    m_keyBindings.emplace(GLFW_KEY_E, TransformBinding{ETransformAction::Rotate, -kRotateStep, 0.0f});
    // 缩放变换
    m_keyBindings.emplace(GLFW_KEY_Z, TransformBinding{ETransformAction::Scale, kScaleStep, kScaleStep});
    m_keyBindings.emplace(GLFW_KEY_X, TransformBinding{ETransformAction::Scale, 1.0f / kScaleStep, 1.0f / kScaleStep});
    // 单轴缩放变换
    m_keyBindings.emplace(GLFW_KEY_F, TransformBinding{ETransformAction::Scale, kScaleStep, 1.0f}); // X轴放大
    m_keyBindings.emplace(GLFW_KEY_H, TransformBinding{ETransformAction::Scale, 1.0f / kScaleStep, 1.0f}); // X轴缩小
    m_keyBindings.emplace(GLFW_KEY_T, TransformBinding{ETransformAction::Scale, 1.0f, kScaleStep}); // Y轴放大
    m_keyBindings.emplace(GLFW_KEY_G, TransformBinding{ETransformAction::Scale, 1.0f, 1.0f / kScaleStep}); // Y轴缩小
    // 剪切变换
    m_keyBindings.emplace(GLFW_KEY_J, TransformBinding{ETransformAction::Shear, kShearStep, 0.0f}); // X方向剪切
    m_keyBindings.emplace(GLFW_KEY_L, TransformBinding{ETransformAction::Shear, -kShearStep, 0.0f}); // X方向反向剪切
    m_keyBindings.emplace(GLFW_KEY_I, TransformBinding{ETransformAction::Shear, 0.0f, kShearStep}); // Y方向剪切
    m_keyBindings.emplace(GLFW_KEY_K, TransformBinding{ETransformAction::Shear, 0.0f, -kShearStep}); // Y方向反向剪切
    // 镜像变换（nm）
    m_keyBindings.emplace(GLFW_KEY_N, TransformBinding{ETransformAction::ReflectX, 0.0f, 0.0f}); // X轴镜像
    m_keyBindings.emplace(GLFW_KEY_M, TransformBinding{ETransformAction::ReflectY, 0.0f, 0.0f}); // Y轴镜像
    // 重置变换
    m_keyBindings.emplace(GLFW_KEY_R, TransformBinding{ETransformAction::Reset, 0.0f, 0.0f});
}
