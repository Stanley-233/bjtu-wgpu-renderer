#ifndef BJTU_WGPU_RENDERER_INPUTMANAGER_H
#define BJTU_WGPU_RENDERER_INPUTMANAGER_H

#include "InputEventLogger.h"
#include "InputEventBus.h"
#include "InputState.h"
#include "policies/AppHotkeyPolicy.h"
#include "policies/CameraMovePolicy.h"
#include "policies/Transform2DPolicy.h"
#include "policies/Transform3DPolicy.h"

class InputManager {
public:
    InputManager() = default;

    void SetDebugEnabled(bool enabled);

    void SetEventBus(InputEventBus* eventBus);

    void EmitKeyEvent(int key, int action, int mods);

    void EmitMouseButtonEvent(int button, int action, int mods);

    void EmitCursorPosEvent(double xpos, double ypos);

private:
    static bool ShouldSuppressNoBindingLog(int key);

    void UpdateDebugSinkConnection();

    InputState         m_inputState{};
    AppHotkeyPolicy    m_appHotkeyPolicy{};
    Transform2DPolicy  m_transform2DPolicy{};
    Transform3DPolicy  m_transform3DPolicy{};
    CameraMovePolicy   m_cameraMovePolicy{};
    InputEventBus*     m_eventBus = nullptr;
    InputEventLogger   m_eventLogger{};
    bool               m_debugEnabled = false;
    bool               m_debugSinkConnected = false;
};

#endif // BJTU_WGPU_RENDERER_INPUTMANAGER_H
