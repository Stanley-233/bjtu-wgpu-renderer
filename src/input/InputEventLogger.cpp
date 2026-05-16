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
    std::cout << "[Transform2DPolicy][Dispatch] TransformActionEvent action="
              << magic_enum::enum_name(event.action)
              << "(" << static_cast<int>(event.action) << ")"
              << ", amountX=" << event.amountX
              << ", amountY=" << event.amountY << std::endl;
}

void InputEventLogger::OnObjectTransform3DEvent(const ObjectTransform3DEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[Transform3DPolicy][Dispatch] ObjectTransform3DEvent mode="
              << magic_enum::enum_name(event.mode)
              << "(" << static_cast<int>(event.mode) << ")"
              << ", x=" << event.x
              << ", y=" << event.y
              << ", z=" << event.z << std::endl;
}

void InputEventLogger::OnTransform2DStateEvent(const Transform2DStateEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[Transform2DPolicy][Dispatch] Transform2DStateEvent"
              << " tx=" << event.translateX
              << ", ty=" << event.translateY
              << ", r=" << event.rotateRate
              << ", sxr=" << event.scaleXRate
              << ", syr=" << event.scaleYRate
              << ", shx=" << event.shearXRate
              << ", shy=" << event.shearYRate << std::endl;
}

void InputEventLogger::OnObjectTransform3DStateEvent(const ObjectTransform3DStateEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[Transform3DPolicy][Dispatch] ObjectTransform3DStateEvent"
              << " tx=" << event.translateX
              << ", ty=" << event.translateY
              << ", tz=" << event.translateZ
              << ", rx=" << event.rotateXRate
              << ", ry=" << event.rotateYRate
              << ", rz=" << event.rotateZRate
              << ", sxr=" << event.scaleXRate
              << ", syr=" << event.scaleYRate
              << ", szr=" << event.scaleZRate << std::endl;
}

void InputEventLogger::OnCameraMoveInputEvent(const CameraMoveInputEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[CameraMovePolicy][Dispatch] CameraMoveInputEvent"
              << " forward=" << event.forward
              << ", right=" << event.right
              << ", up=" << event.up << std::endl;
}

void InputEventLogger::OnCameraLookInputEvent(const CameraLookInputEvent& event) const {
    if (!m_enabled) {
        return;
    }
    std::cout << "[InputManager][Dispatch] CameraLookInputEvent"
              << " deltaYaw=" << event.deltaYaw
              << ", deltaPitch=" << event.deltaPitch
              << ", isLookModeActive=" << (event.isLookModeActive ? "true" : "false") << std::endl;
}

const char* InputEventLogger::KeyName(const int key) {
    switch (key) {
        case GLFW_KEY_1:
            return "GLFW_KEY_1";
        case GLFW_KEY_2:
            return "GLFW_KEY_2";
        case GLFW_KEY_3:
            return "GLFW_KEY_3";
        case GLFW_KEY_C:
            return "GLFW_KEY_C";
        case GLFW_KEY_LEFT_SHIFT:
            return "GLFW_KEY_LEFT_SHIFT";
        case GLFW_KEY_RIGHT_SHIFT:
            return "GLFW_KEY_RIGHT_SHIFT";
        case GLFW_KEY_LEFT_ALT:
            return "GLFW_KEY_LEFT_ALT";
        case GLFW_KEY_RIGHT_ALT:
            return "GLFW_KEY_RIGHT_ALT";
        case GLFW_KEY_LEFT_CONTROL:
            return "GLFW_KEY_LEFT_CONTROL";
        case GLFW_KEY_RIGHT_CONTROL:
            return "GLFW_KEY_RIGHT_CONTROL";
        case GLFW_KEY_Q:
            return "GLFW_KEY_Q";
        case GLFW_KEY_E:
            return "GLFW_KEY_E";
        case GLFW_KEY_W:
            return "GLFW_KEY_W";
        case GLFW_KEY_A:
            return "GLFW_KEY_A";
        case GLFW_KEY_S:
            return "GLFW_KEY_S";
        case GLFW_KEY_D:
            return "GLFW_KEY_D";
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
        case GLFW_KEY_O:
            return "GLFW_KEY_O";
        case GLFW_KEY_U:
            return "GLFW_KEY_U";
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