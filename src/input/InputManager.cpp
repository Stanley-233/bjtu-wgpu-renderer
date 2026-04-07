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
    m_eventLogger.SetEnabled(enabled);
    if (enabled && !m_debugSinkConnected) {
        m_dispatcher.sink<TransformActionEvent>().connect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_dispatcher.sink<ObjectTransform3DEvent>().connect<&InputEventLogger::OnObjectTransform3DEvent>(m_eventLogger);
        m_dispatcher.sink<Transform2DStateEvent>().connect<&InputEventLogger::OnTransform2DStateEvent>(m_eventLogger);
        m_dispatcher.sink<ObjectTransform3DStateEvent>().connect<&InputEventLogger::OnObjectTransform3DStateEvent>(m_eventLogger);
        m_dispatcher.sink<CameraMoveInputEvent>().connect<&InputEventLogger::OnCameraMoveInputEvent>(m_eventLogger);
        m_debugSinkConnected = true;
    } else if (!enabled && m_debugSinkConnected) {
        m_dispatcher.sink<TransformActionEvent>().disconnect<&InputEventLogger::OnTransformActionEvent>(m_eventLogger);
        m_dispatcher.sink<ObjectTransform3DEvent>().disconnect<&InputEventLogger::OnObjectTransform3DEvent>(m_eventLogger);
        m_dispatcher.sink<Transform2DStateEvent>().disconnect<&InputEventLogger::OnTransform2DStateEvent>(m_eventLogger);
        m_dispatcher.sink<ObjectTransform3DStateEvent>().disconnect<&InputEventLogger::OnObjectTransform3DStateEvent>(m_eventLogger);
        m_dispatcher.sink<CameraMoveInputEvent>().disconnect<&InputEventLogger::OnCameraMoveInputEvent>(m_eventLogger);
        m_debugSinkConnected = false;
    }
}

void InputManager::EmitKeyEvent(const int key, const int action, const int mods) {
    m_eventLogger.LogRawKeyEvent(key, action, mods);

    const InputStateUpdate update = m_inputState.ApplyKeyEvent(key, action, mods);
    const bool             isKeyEvent = update.isPressLike || update.isRelease;
    if (!isKeyEvent) {
        return;
    }

    bool handledPress = false;
    (void)m_cameraMovePolicy.Process(m_inputState, update, key, action, m_dispatcher, handledPress);
    (void)m_transform2DPolicy.Process(m_inputState, update, key, action, m_dispatcher, handledPress);
    (void)m_transform3DPolicy.Process(m_inputState, update, key, action, m_dispatcher, handledPress);

    if (action == GLFW_PRESS
        && !handledPress
        && !m_cameraMovePolicy.HandlesKey(key)
        && !m_transform2DPolicy.HandlesKey(key)
        && !m_transform3DPolicy.HandlesKey(key)
        && !ShouldSuppressNoBindingLog(key)) {
        m_eventLogger.LogNoBinding(key);
    }
}

void InputManager::SubscribeTransform2DInput(ITransform2DInputSink& sink) {
    m_dispatcher.sink<TransformActionEvent>().connect<&ITransform2DInputSink::OnTransformInputEvent>(sink);
    m_dispatcher.sink<Transform2DStateEvent>().connect<&ITransform2DInputSink::OnTransform2DStateEvent>(sink);
}

void InputManager::UnsubscribeTransform2DInput(ITransform2DInputSink& sink) {
    m_dispatcher.sink<TransformActionEvent>().disconnect<&ITransform2DInputSink::OnTransformInputEvent>(sink);
    m_dispatcher.sink<Transform2DStateEvent>().disconnect<&ITransform2DInputSink::OnTransform2DStateEvent>(sink);
}

void InputManager::SubscribeTransform3DInput(ITransform3DInputSink& sink) {
    m_dispatcher.sink<ObjectTransform3DEvent>().connect<&ITransform3DInputSink::OnObjectTransform3DEvent>(sink);
    m_dispatcher.sink<ObjectTransform3DStateEvent>().connect<&ITransform3DInputSink::OnObjectTransform3DStateEvent>(sink);
}

void InputManager::UnsubscribeTransform3DInput(ITransform3DInputSink& sink) {
    m_dispatcher.sink<ObjectTransform3DEvent>().disconnect<&ITransform3DInputSink::OnObjectTransform3DEvent>(sink);
    m_dispatcher.sink<ObjectTransform3DStateEvent>().disconnect<&ITransform3DInputSink::OnObjectTransform3DStateEvent>(sink);
}

void InputManager::SubscribeCameraMoveInput(ICameraMoveInputSink& sink) {
    m_dispatcher.sink<CameraMoveInputEvent>().connect<&ICameraMoveInputSink::OnCameraMoveInputEvent>(sink);
}

void InputManager::UnsubscribeCameraMoveInput(ICameraMoveInputSink& sink) {
    m_dispatcher.sink<CameraMoveInputEvent>().disconnect<&ICameraMoveInputSink::OnCameraMoveInputEvent>(sink);
}
