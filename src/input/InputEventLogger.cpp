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
        case GLFW_KEY_Q:
            return "GLFW_KEY_Q";
        case GLFW_KEY_E:
            return "GLFW_KEY_E";
        case GLFW_KEY_Z:
            return "GLFW_KEY_Z";
        case GLFW_KEY_X:
            return "GLFW_KEY_X";
        case GLFW_KEY_T:
            return "GLFW_KEY_T";
        case GLFW_KEY_F:
            return "GLFW_KEY_F";
        case GLFW_KEY_G:
            return "GLFW_KEY_G";
        case GLFW_KEY_H:
            return "GLFW_KEY_H";
        case GLFW_KEY_I:
            return "GLFW_KEY_I";
        case GLFW_KEY_J:
            return "GLFW_KEY_J";
        case GLFW_KEY_K:
            return "GLFW_KEY_K";
        case GLFW_KEY_L:
            return "GLFW_KEY_L";
        case GLFW_KEY_R:
            return "GLFW_KEY_R";
        case GLFW_KEY_N:
            return "GLFW_KEY_N";
        case GLFW_KEY_M:
            return "GLFW_KEY_M";
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
