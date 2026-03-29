#include "InputEventLogger.h"

#include <GLFW/glfw3.h>
#include <magic_enum.hpp>

#include <iostream>

void InputEventLogger::SetEnabled(const bool enabled) {
    m_enabled = enabled;
}

bool InputEventLogger::IsEnabled() const {
    return m_enabled;
}

void InputEventLogger::LogRawKeyEvent(const int key, const int action, const int mods) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[InputManager][Emit] key=" << KeyName(key) << "(" << key << ")"
              << ", action=" << ActionName(action) << "(" << action << ")"
              << ", mods=" << mods << std::endl;
}

void InputEventLogger::LogNoBinding(const int key) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[InputManager][Emit] no transform binding for key="
              << KeyName(key) << "(" << key << ")" << std::endl;
}

void InputEventLogger::OnTransformActionEvent(const TransformActionEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[InputManager][Dispatch] TransformActionEvent action="
              << magic_enum::enum_name(event.action)
              << "(" << static_cast<int>(event.action) << ")"
              << ", amountX=" << event.amountX
              << ", amountY=" << event.amountY << std::endl;
}

const char* InputEventLogger::KeyName(const int key) {
    switch (key) {
        case GLFW_KEY_W:
            return "GLFW_KEY_W";
        case GLFW_KEY_A:
            return "GLFW_KEY_A";
        case GLFW_KEY_S:
            return "GLFW_KEY_S";
        case GLFW_KEY_D:
            return "GLFW_KEY_D";
        default:
            return "GLFW_KEY_UNKNOWN";
    }
}

const char* InputEventLogger::ActionName(const int action) {
    switch (action) {
        case GLFW_PRESS:
            return "GLFW_PRESS";
        case GLFW_RELEASE:
            return "GLFW_RELEASE";
        case GLFW_REPEAT:
            return "GLFW_REPEAT";
        default:
            return "GLFW_ACTION_UNKNOWN";
    }
}
