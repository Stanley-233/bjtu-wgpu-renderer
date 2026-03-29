#ifndef BJTU_WGPU_RENDERER_INPUTMANAGER_H
#define BJTU_WGPU_RENDERER_INPUTMANAGER_H

#include <unordered_map>

#include <signal/dispatcher.hpp>

#include "InputEventLogger.h"
#include "../scene/Scene.h"

class InputManager {
public:
    InputManager();

    void SetDebugEnabled(bool enabled);

    void EmitKeyEvent(int key, int action, int mods);

    void SubscribeTransformActions(IScene& scene);

    void UnsubscribeTransformActions(IScene& scene);

private:
    struct TransformBinding {
        ETransformAction action;
        float            amountX;
        float            amountY;
    };

    void InitializeDefaultBindings();

    std::unordered_map<int, TransformBinding> m_keyBindings;
    entt::dispatcher                          m_dispatcher;
    InputEventLogger                          m_eventLogger;
    bool                                      m_debugSinkConnected = false;
};

#endif // BJTU_WGPU_RENDERER_INPUTMANAGER_H
