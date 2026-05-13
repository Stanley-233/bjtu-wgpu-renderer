#ifndef BJTU_WGPU_RENDERER_INPUTEVENTLOGGER_H
#define BJTU_WGPU_RENDERER_INPUTEVENTLOGGER_H

#include "scene/IScene.h"

class InputEventLogger {
public:
    void SetEnabled(bool enabled);

    [[nodiscard]] bool IsEnabled() const;

    void LogRawKeyEvent(int key, int action, int mods) const;

    void LogNoBinding(int key) const;

    void OnTransformActionEvent(const TransformActionEvent& event) const;

    void OnObjectTransform3DEvent(const ObjectTransform3DEvent& event) const;

    void OnTransform2DStateEvent(const Transform2DStateEvent& event) const;

    void OnObjectTransform3DStateEvent(const ObjectTransform3DStateEvent& event) const;

    void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) const;

private:
    static const char* KeyName(int key);

    static const char* ActionName(int action);

    bool m_enabled = false;
};

#endif // BJTU_WGPU_RENDERER_INPUTEVENTLOGGER_H
