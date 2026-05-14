#include "InputManager.h"

#include <GLFW/glfw3.h>

bool InputManager::ShouldSuppressNoBindingLog(const int key) {
    switch (key) {
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_C:
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
        case GLFW_KEY_LEFT_ALT:
        case GLFW_KEY_RIGHT_ALT:
        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
        case GLFW_KEY_W:
        case GLFW_KEY_A:
        case GLFW_KEY_S:
        case GLFW_KEY_D:
        case GLFW_KEY_Q:
        case GLFW_KEY_E:
            return true;
        default:
            return false;
    }
}

void InputManager::SetDebugEnabled(const bool enabled) {
    m_debugEnabled = enabled;
    m_eventLogger.SetEnabled(enabled);
    UpdateDebugSinkConnection();
}

void InputManager::SetEventBus(InputEventBus* eventBus) {
    if (m_eventBus == eventBus) {
        return;
    }
    if (m_eventBus != nullptr && m_debugSinkConnected) {
        m_eventBus->Dispatcher().sink<TransformActionEvent>().disconnect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DEvent>().disconnect<&InputEventLogger::OnObjectTransform3DEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<Transform2DStateEvent>().disconnect<&InputEventLogger::OnTransform2DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DStateEvent>().disconnect<&InputEventLogger::OnObjectTransform3DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<CameraMoveInputEvent>().disconnect<&InputEventLogger::OnCameraMoveInputEvent>(m_eventLogger);
        // TODO：后续实现 CameraLookInputEvent 分发后，在这里补充鼠标视角输入日志的解绑。
        m_debugSinkConnected = false;
    }
    m_eventBus = eventBus;
    UpdateDebugSinkConnection();
}

void InputManager::UpdateDebugSinkConnection() {
    if (m_eventBus == nullptr) {
        m_debugSinkConnected = false;
        return;
    }
    if (m_debugEnabled && !m_debugSinkConnected) {
        m_eventBus->Dispatcher().sink<TransformActionEvent>().connect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DEvent>().connect<&InputEventLogger::OnObjectTransform3DEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<Transform2DStateEvent>().connect<&InputEventLogger::OnTransform2DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DStateEvent>().connect<&InputEventLogger::OnObjectTransform3DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<CameraMoveInputEvent>().connect<&InputEventLogger::OnCameraMoveInputEvent>(m_eventLogger);
        // TODO：后续开始发出鼠标视角事件后，在这里补充 CameraLookInputEvent 日志绑定。
        m_debugSinkConnected = true;
    } else if (!m_debugEnabled && m_debugSinkConnected) {
        m_eventBus->Dispatcher().sink<TransformActionEvent>().disconnect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DEvent>().disconnect<&InputEventLogger::OnObjectTransform3DEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<Transform2DStateEvent>().disconnect<&InputEventLogger::OnTransform2DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<ObjectTransform3DStateEvent>().disconnect<&InputEventLogger::OnObjectTransform3DStateEvent>(m_eventLogger);
        m_eventBus->Dispatcher().sink<CameraMoveInputEvent>().disconnect<&InputEventLogger::OnCameraMoveInputEvent>(m_eventLogger);
        // TODO：后续开始发出鼠标视角事件后，在这里补充 CameraLookInputEvent 日志解绑。
        m_debugSinkConnected = false;
    }
}

void InputManager::EmitKeyEvent(const int key, const int action, const int mods) {
    m_eventLogger.LogRawKeyEvent(key, action, mods);
    if (m_eventBus == nullptr) {
        return;
    }

    const InputStateUpdate update = m_inputState.ApplyKeyEvent(key, action, mods);
    const bool             isKeyEvent = update.isPressLike || update.isRelease;
    if (!isKeyEvent) {
        return;
    }

    bool handledPress = false;
    entt::dispatcher& dispatcher = m_eventBus->Dispatcher();
    (void)m_appHotkeyPolicy.Process(m_inputState, update, key, action, dispatcher, handledPress);
    (void)m_cameraMovePolicy.Process(m_inputState, update, key, action, dispatcher, handledPress);
    (void)m_transform2DPolicy.Process(m_inputState, update, key, action, dispatcher, handledPress);
    (void)m_transform3DPolicy.Process(m_inputState, update, key, action, dispatcher, handledPress);

    if (action == GLFW_PRESS
        && !handledPress
        && !m_appHotkeyPolicy.HandlesKey(key)
        && !m_cameraMovePolicy.HandlesKey(key)
        && !m_transform2DPolicy.HandlesKey(key)
        && !m_transform3DPolicy.HandlesKey(key)
        && !ShouldSuppressNoBindingLog(key)) {
        // TODO：后续鼠标视角绑定应使用独立的鼠标诊断日志，而不是复用这里的键盘兜底日志。
        m_eventLogger.LogNoBinding(key);
    }
}

void InputManager::EmitMouseButtonEvent(const int button, const int action, const int mods) {
    (void)button;
    (void)action;
    (void)mods;
    // TODO：后续在这里把 RMB 按下/释放映射为视角控制启停，并发出专用鼠标视角事件。
}

void InputManager::EmitCursorPosEvent(const double xpos, const double ypos) {
    (void)xpos;
    (void)ypos;
    // TODO：后续在这里累计鼠标位移，并在 UE 风格 RMB 视角模式下发出 CameraLookInputEvent。
}
