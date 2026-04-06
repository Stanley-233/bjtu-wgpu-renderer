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

    void SubscribeCameraMoveInput(IScene& scene);

    void UnsubscribeCameraMoveInput(IScene& scene);

private:
    struct TransformBinding {
        ETransformAction action;
        float            amountX;
        float            amountY;
    };

    struct CameraMoveBinding {
        float forward;
        float right;
        float up;
    };

    void InitializeDefaultBindings();

    std::unordered_map<int, TransformBinding>  m_keyBindings;
    std::unordered_map<int, CameraMoveBinding> m_cameraMoveBindings;
    std::unordered_map<int, bool>              m_cameraMovePressed;
    entt::dispatcher                           m_dispatcher;
    InputEventLogger                           m_eventLogger;
    float                                      m_moveForward = 0.0f;
    float                                      m_moveRight   = 0.0f;
    float                                      m_moveUp      = 0.0f;
    bool                                       m_debugSinkConnected = false;
};

#endif // BJTU_WGPU_RENDERER_INPUTMANAGER_H
