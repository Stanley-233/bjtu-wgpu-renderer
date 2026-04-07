#ifndef BJTU_WGPU_RENDERER_CAMERAMOVEPOLICY_H
#define BJTU_WGPU_RENDERER_CAMERAMOVEPOLICY_H

#include <algorithm>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <signal/dispatcher.hpp>

#include "../../scene/Scene.h"
#include "../InputState.h"
#include "InputPolicy.h"

class CameraMovePolicy final : public InputPolicy {
public:
    struct CameraBinding {
        float forward;
        float right;
        float up;
    };

    CameraMovePolicy() {
        InitializeDefaultBindings();
    }

    bool Process(
        const InputState& state,
        const InputStateUpdate& update,
        const int key,
        const int action,
        entt::dispatcher& dispatcher,
        bool& handledPress) override {
        (void)action;
        if (!update.keyStateChanged || !HandlesKey(key)) {
            return false;
        }

        float forward = 0.0f;
        float right   = 0.0f;
        float up      = 0.0f;
        for (const auto& [boundKey, binding] : m_bindings) {
            if (!state.IsPressed(boundKey)) {
                continue;
            }
            forward += binding.forward;
            right += binding.right;
            up += binding.up;
        }
        const CameraMoveInputEvent next{
            .forward = std::clamp(forward, -1.0f, 1.0f),
            .right   = std::clamp(right, -1.0f, 1.0f),
            .up      = std::clamp(up, -1.0f, 1.0f),
        };
        if (next.forward == m_state.forward && next.right == m_state.right && next.up == m_state.up) {
            return false;
        }
        m_state = next;
        dispatcher.trigger<CameraMoveInputEvent>(CameraMoveInputEvent{m_state});
        if (state.IsPressed(key)) {
            handledPress = true;
        }
        return true;
    }

    [[nodiscard]] bool HandlesKey(const int key) const override {
        return m_bindings.contains(key);
    }

private:
    void InitializeDefaultBindings() {
        m_bindings.clear();
        m_state = {};
        m_bindings.emplace(GLFW_KEY_W, CameraBinding{1.0f, 0.0f, 0.0f});
        m_bindings.emplace(GLFW_KEY_S, CameraBinding{-1.0f, 0.0f, 0.0f});
        m_bindings.emplace(GLFW_KEY_A, CameraBinding{0.0f, -1.0f, 0.0f});
        m_bindings.emplace(GLFW_KEY_D, CameraBinding{0.0f, 1.0f, 0.0f});
        m_bindings.emplace(GLFW_KEY_Q, CameraBinding{0.0f, 0.0f, 1.0f});
        m_bindings.emplace(GLFW_KEY_E, CameraBinding{0.0f, 0.0f, -1.0f});
    }

    std::unordered_map<int, CameraBinding> m_bindings{};
    CameraMoveInputEvent                   m_state{};
};

#endif // BJTU_WGPU_RENDERER_CAMERAMOVEPOLICY_H
