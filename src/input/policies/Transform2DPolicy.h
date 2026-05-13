#ifndef BJTU_WGPU_RENDERER_TRANSFORM2DPOLICY_H
#define BJTU_WGPU_RENDERER_TRANSFORM2DPOLICY_H

#include <cmath>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <signal/dispatcher.hpp>

#include "scene/IScene.h"
#include "input/InputState.h"
#include "InputPolicy.h"

class Transform2DPolicy final : public InputPolicy {
public:
    struct Binding2D {
        float x;
        float y;
    };

    Transform2DPolicy() {
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

        if (action == GLFW_PRESS) {
            const auto it = m_discreteBindings.find(key);
            if (it != m_discreteBindings.end()) {
                dispatcher.trigger<TransformActionEvent>({
                    it->second.action,
                    it->second.amountX,
                    it->second.amountY
                });
                handledPress = true;
            }
        }
        return changed;
    }

    [[nodiscard]] bool HandlesKey(const int key) const override {
        return HandlesContinuousKey(key) || m_discreteBindings.contains(key);
    }

private:
    struct DiscreteBinding {
        ETransformAction action;
        float            amountX;
        float            amountY;
    };

    static constexpr float kReferenceFps = 60.0f;
    static constexpr float kTranslateStep = 0.05f;
    static constexpr float kRotateStep    = 0.1f;
    static constexpr float kScaleStep     = 1.1f;
    static constexpr float kShearStep     = 0.1f;

    static constexpr float kTranslateRate = kTranslateStep * kReferenceFps;
    static constexpr float kRotateRate    = kRotateStep * kReferenceFps;
    static constexpr float kShearRate     = kShearStep * kReferenceFps;
    const float            kScaleLnRate   = std::log(kScaleStep) * kReferenceFps;

    void InitializeDefaultBindings() {
        m_discreteBindings.clear();
        m_translateBindings.clear();
        m_rotateBindings.clear();
        m_scaleBindings.clear();
        m_shearBindings.clear();
        m_state = {};

        // none: translation
        m_translateBindings.emplace(GLFW_KEY_H, Binding2D{-kTranslateRate, 0.0f});
        m_translateBindings.emplace(GLFW_KEY_J, Binding2D{kTranslateRate, 0.0f});
        m_translateBindings.emplace(GLFW_KEY_U, Binding2D{0.0f, kTranslateRate});
        m_translateBindings.emplace(GLFW_KEY_O, Binding2D{0.0f, -kTranslateRate});

        // rotation: i/k (no modifier required)
        m_rotateBindings.emplace(GLFW_KEY_I, Binding2D{kRotateRate, 0.0f});
        m_rotateBindings.emplace(GLFW_KEY_K, Binding2D{-kRotateRate, 0.0f});

        // alt: scale
        m_scaleBindings.emplace(GLFW_KEY_H, Binding2D{-kScaleLnRate, 0.0f});
        m_scaleBindings.emplace(GLFW_KEY_J, Binding2D{kScaleLnRate, 0.0f});
        m_scaleBindings.emplace(GLFW_KEY_U, Binding2D{0.0f, kScaleLnRate});
        m_scaleBindings.emplace(GLFW_KEY_O, Binding2D{0.0f, -kScaleLnRate});

        // ctrl: shear
        m_shearBindings.emplace(GLFW_KEY_H, Binding2D{kShearRate, 0.0f});
        m_shearBindings.emplace(GLFW_KEY_J, Binding2D{-kShearRate, 0.0f});
        m_shearBindings.emplace(GLFW_KEY_U, Binding2D{0.0f, kShearRate});
        m_shearBindings.emplace(GLFW_KEY_O, Binding2D{0.0f, -kShearRate});

        // discrete
        m_discreteBindings.emplace(GLFW_KEY_N, DiscreteBinding{ETransformAction::ReflectX, 0.0f, 0.0f});
        m_discreteBindings.emplace(GLFW_KEY_M, DiscreteBinding{ETransformAction::ReflectY, 0.0f, 0.0f});
        m_discreteBindings.emplace(GLFW_KEY_R, DiscreteBinding{ETransformAction::Reset, 0.0f, 0.0f});
    }

    [[nodiscard]] bool HandlesContinuousKey(const int key) const {
        return m_translateBindings.contains(key)
               || m_rotateBindings.contains(key)
               || m_scaleBindings.contains(key)
               || m_shearBindings.contains(key);
    }

    bool RecomputeAndDispatch(const InputState& state, entt::dispatcher& dispatcher) {
        Transform2DStateEvent next{};

        // 2D mapping: Alt(scale), Ctrl(shear), default(translate + rotate).
        // Shift is intentionally ignored for 2D rotation.
        if (state.altPressed) {
            for (const auto& [key, binding] : m_scaleBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.scaleXRate += binding.x;
                next.scaleYRate += binding.y;
            }
        } else if (state.ctrlPressed) {
            for (const auto& [key, binding] : m_shearBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.shearXRate += binding.x;
                next.shearYRate += binding.y;
            }
        } else {
            for (const auto& [key, binding] : m_translateBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.translateX += binding.x;
                next.translateY += binding.y;
            }
            for (const auto& [key, binding] : m_rotateBindings) {
                if (!state.IsPressed(key)) {
                    continue;
                }
                next.rotateRate += binding.x;
            }
        }

        const bool changed = next.translateX != m_state.translateX
                             || next.translateY != m_state.translateY
                             || next.rotateRate != m_state.rotateRate
                             || next.scaleXRate != m_state.scaleXRate
                             || next.scaleYRate != m_state.scaleYRate
                             || next.shearXRate != m_state.shearXRate
                             || next.shearYRate != m_state.shearYRate;
        if (!changed) {
            return false;
        }
        m_state = next;
        dispatcher.trigger<Transform2DStateEvent>(Transform2DStateEvent{m_state});
        return true;
    }

    std::unordered_map<int, DiscreteBinding> m_discreteBindings{};
    std::unordered_map<int, Binding2D>       m_translateBindings{};
    std::unordered_map<int, Binding2D>       m_rotateBindings{};
    std::unordered_map<int, Binding2D>       m_scaleBindings{};
    std::unordered_map<int, Binding2D>       m_shearBindings{};
    Transform2DStateEvent                    m_state{};
};

#endif // BJTU_WGPU_RENDERER_TRANSFORM2DPOLICY_H
