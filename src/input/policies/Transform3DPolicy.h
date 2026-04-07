#ifndef BJTU_WGPU_RENDERER_TRANSFORM3DPOLICY_H
#define BJTU_WGPU_RENDERER_TRANSFORM3DPOLICY_H

#include <cmath>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <signal/dispatcher.hpp>

#include "../../scene/Scene.h"
#include "../InputState.h"
#include "InputPolicy.h"

class Transform3DPolicy final : public InputPolicy {
public:
    struct Binding3D {
        float x;
        float y;
        float z;
    };

    Transform3DPolicy() {
        InitializeDefaultBindings();
    }

    bool Process(
        const InputState& state,
        const InputStateUpdate& update,
        const int key,
        const int action,
        entt::dispatcher& dispatcher,
        bool& handledPress) override {
        bool changed = false;
        if ((update.keyStateChanged && HandlesContinuousKey(key)) || update.modifierChanged) {
            changed = RecomputeAndDispatch(state, dispatcher);
        }

        if (action == GLFW_PRESS && key == GLFW_KEY_R) {
            dispatcher.trigger<ObjectTransform3DEvent>({EObjectTransform3DMode::Reset, 0.0f, 0.0f, 0.0f});
            handledPress = true;
        }

        return changed;
    }

    [[nodiscard]] bool HandlesKey(const int key) const override {
        return key == GLFW_KEY_R || HandlesContinuousKey(key);
    }

private:
    static constexpr float kReferenceFps = 60.0f;
    static constexpr float kTranslateStep = 0.05f;
    static constexpr float kRotateStep    = 0.1f;
    static constexpr float kScaleStep     = 1.1f;

    static constexpr float kTranslateRate = kTranslateStep * kReferenceFps;
    static constexpr float kRotateRate    = kRotateStep * kReferenceFps;
    const float            kScaleLnRate   = std::log(kScaleStep) * kReferenceFps;

    void InitializeDefaultBindings() {
        m_translateBindings.clear();
        m_rotateBindings.clear();
        m_scaleBindings.clear();
        m_state = {};

        m_translateBindings.emplace(GLFW_KEY_H, Binding3D{-kTranslateRate, 0.0f, 0.0f});
        m_translateBindings.emplace(GLFW_KEY_J, Binding3D{kTranslateRate, 0.0f, 0.0f});
        m_translateBindings.emplace(GLFW_KEY_I, Binding3D{0.0f, 0.0f, kTranslateRate});
        m_translateBindings.emplace(GLFW_KEY_K, Binding3D{0.0f, 0.0f, -kTranslateRate});
        m_translateBindings.emplace(GLFW_KEY_U, Binding3D{0.0f, kTranslateRate, 0.0f});
        m_translateBindings.emplace(GLFW_KEY_O, Binding3D{0.0f, -kTranslateRate, 0.0f});

        m_rotateBindings.emplace(GLFW_KEY_H, Binding3D{-kRotateRate, 0.0f, 0.0f});
        m_rotateBindings.emplace(GLFW_KEY_J, Binding3D{kRotateRate, 0.0f, 0.0f});
        m_rotateBindings.emplace(GLFW_KEY_I, Binding3D{0.0f, 0.0f, kRotateRate});
        m_rotateBindings.emplace(GLFW_KEY_K, Binding3D{0.0f, 0.0f, -kRotateRate});
        m_rotateBindings.emplace(GLFW_KEY_U, Binding3D{0.0f, kRotateRate, 0.0f});
        m_rotateBindings.emplace(GLFW_KEY_O, Binding3D{0.0f, -kRotateRate, 0.0f});

        m_scaleBindings.emplace(GLFW_KEY_H, Binding3D{-kScaleLnRate, 0.0f, 0.0f});
        m_scaleBindings.emplace(GLFW_KEY_J, Binding3D{kScaleLnRate, 0.0f, 0.0f});
        m_scaleBindings.emplace(GLFW_KEY_I, Binding3D{0.0f, 0.0f, kScaleLnRate});
        m_scaleBindings.emplace(GLFW_KEY_K, Binding3D{0.0f, 0.0f, -kScaleLnRate});
        m_scaleBindings.emplace(GLFW_KEY_U, Binding3D{0.0f, kScaleLnRate, 0.0f});
        m_scaleBindings.emplace(GLFW_KEY_O, Binding3D{0.0f, -kScaleLnRate, 0.0f});
    }

    [[nodiscard]] bool HandlesContinuousKey(const int key) const {
        return m_translateBindings.contains(key);
    }

    bool RecomputeAndDispatch(const InputState& state, entt::dispatcher& dispatcher) {
        ObjectTransform3DStateEvent next{};

        if (state.shiftPressed) {
            for (const auto& [key, binding] : m_rotateBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.rotateXRate += binding.x;
                next.rotateYRate += binding.y;
                next.rotateZRate += binding.z;
            }
        } else if (state.altPressed) {
            for (const auto& [key, binding] : m_scaleBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.scaleXRate += binding.x;
                next.scaleYRate += binding.y;
                next.scaleZRate += binding.z;
            }
        } else {
            for (const auto& [key, binding] : m_translateBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.translateX += binding.x;
                next.translateY += binding.y;
                next.translateZ += binding.z;
            }
        }

        const bool changed = next.translateX != m_state.translateX
                             || next.translateY != m_state.translateY
                             || next.translateZ != m_state.translateZ
                             || next.rotateXRate != m_state.rotateXRate
                             || next.rotateYRate != m_state.rotateYRate
                             || next.rotateZRate != m_state.rotateZRate
                             || next.scaleXRate != m_state.scaleXRate
                             || next.scaleYRate != m_state.scaleYRate
                             || next.scaleZRate != m_state.scaleZRate;
        if (!changed) {
            return false;
        }
        m_state = next;
        dispatcher.trigger<ObjectTransform3DStateEvent>(ObjectTransform3DStateEvent{m_state});
        return true;
    }

    std::unordered_map<int, Binding3D> m_translateBindings{};
    std::unordered_map<int, Binding3D> m_rotateBindings{};
    std::unordered_map<int, Binding3D> m_scaleBindings{};
    ObjectTransform3DStateEvent        m_state{};
};

#endif // BJTU_WGPU_RENDERER_TRANSFORM3DPOLICY_H
