#ifndef BJTU_WGPU_RENDERER_CAMERALOOKPOLICY_H
#define BJTU_WGPU_RENDERER_CAMERALOOKPOLICY_H

#include <GLFW/glfw3.h>
#include <signal/dispatcher.hpp>

#include "scene/IScene.h"

class CameraLookPolicy {
public:
    void OnMouseButton(const int button, const int action, entt::dispatcher& dispatcher) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                m_rmbPressed = true;
            } else if (action == GLFW_RELEASE) {
                m_rmbPressed = false;
                dispatcher.trigger<CameraLookInputEvent>(CameraLookInputEvent{0.0f, 0.0f, false});
            }
        }
    }

    void OnCursorPos(const double xpos, const double ypos, entt::dispatcher& dispatcher) {
        if (m_lastX == 0.0 && m_lastY == 0.0) {
            m_lastX = xpos;
            m_lastY = ypos;
            return;
        }
        if (m_rmbPressed) {
            const float deltaX = static_cast<float>(xpos - m_lastX);
            const float deltaY = static_cast<float>(m_lastY - ypos);
            dispatcher.trigger<CameraLookInputEvent>(CameraLookInputEvent{deltaX, deltaY, true});
        }
        m_lastX = xpos;
        m_lastY = ypos;
    }

private:
    bool   m_rmbPressed = false;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
};

#endif // BJTU_WGPU_RENDERER_CAMERALOOKPOLICY_H
