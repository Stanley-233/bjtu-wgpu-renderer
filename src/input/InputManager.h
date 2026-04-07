#ifndef BJTU_WGPU_RENDERER_INPUTMANAGER_H
#define BJTU_WGPU_RENDERER_INPUTMANAGER_H

#include <signal/dispatcher.hpp>

#include "InputEventLogger.h"
#include "InputState.h"
#include "policies/CameraMovePolicy.h"
#include "policies/Transform2DPolicy.h"
#include "policies/Transform3DPolicy.h"

class InputManager {
public:
    InputManager() = default;

    void SetDebugEnabled(bool enabled);

    void EmitKeyEvent(int key, int action, int mods);

    void SubscribeTransform2DInput(ITransform2DInputSink& sink);

    void UnsubscribeTransform2DInput(ITransform2DInputSink& sink);

    void SubscribeTransform3DInput(ITransform3DInputSink& sink);

    void UnsubscribeTransform3DInput(ITransform3DInputSink& sink);

    void SubscribeCameraMoveInput(ICameraMoveInputSink& sink);

    void UnsubscribeCameraMoveInput(ICameraMoveInputSink& sink);

private:
    static bool ShouldSuppressNoBindingLog(int key);

    InputState         m_inputState{};
    Transform2DPolicy  m_transform2DPolicy{};
    Transform3DPolicy  m_transform3DPolicy{};
    CameraMovePolicy   m_cameraMovePolicy{};
    entt::dispatcher   m_dispatcher{};
    InputEventLogger   m_eventLogger{};
    bool               m_debugSinkConnected = false;
};

#endif // BJTU_WGPU_RENDERER_INPUTMANAGER_H
