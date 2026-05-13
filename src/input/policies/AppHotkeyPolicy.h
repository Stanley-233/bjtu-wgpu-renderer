#ifndef BJTU_WGPU_RENDERER_APPHOTKEYPOLICY_H
#define BJTU_WGPU_RENDERER_APPHOTKEYPOLICY_H

#include <GLFW/glfw3.h>

#include "scene/IScene.h"
#include "input/InputState.h"
#include "InputPolicy.h"

class AppHotkeyPolicy final : public InputPolicy {
public:
    bool Process(
        const InputState& state,
        const InputStateUpdate& update,
        const int key,
        const int action,
        entt::dispatcher& dispatcher,
        bool& handledPress) override {
        (void)state;
        (void)update;
        if (action != GLFW_PRESS) {
            return false;
        }

        if (key == GLFW_KEY_1) {
            dispatcher.trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::Scene2D});
            handledPress = true;
            return true;
        }
        if (key == GLFW_KEY_2) {
            dispatcher.trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::Scene3DLegacy});
            handledPress = true;
            return true;
        }
        if (key == GLFW_KEY_3) {
            dispatcher.trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::LogicScene});
            handledPress = true;
            return true;
        }
        if (key == GLFW_KEY_C) {
            dispatcher.trigger<ToggleCameraModeRequest>(ToggleCameraModeRequest{});
            handledPress = true;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool HandlesKey(const int key) const override {
        return key == GLFW_KEY_1 || key == GLFW_KEY_2 || key == GLFW_KEY_3 || key == GLFW_KEY_C;
    }
};

#endif // BJTU_WGPU_RENDERER_APPHOTKEYPOLICY_H
