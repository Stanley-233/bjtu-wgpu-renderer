#ifndef BJTU_WGPU_RENDERER_SCENE_H
#define BJTU_WGPU_RENDERER_SCENE_H

class RenderContext;
class InputEventBus;
class LegacyGuiRenderer;

enum class ESceneType {
    Scene2D,
    Scene3DLegacy,
    LogicScene,
};

enum class ETransformAction {
    Translate,
    Rotate,
    Scale,
    Shear,
    ReflectX,
    ReflectY,
    Reset,
};

struct TransformActionEvent {
    ETransformAction action;
    float            amountX;
    float            amountY;
};

enum class EObjectTransform3DMode {
    Translate,
    Rotate,
    Scale,
    Reset,
};

struct ObjectTransform3DEvent {
    EObjectTransform3DMode mode;
    float                  x;
    float                  y;
    float                  z;
};

struct Transform2DStateEvent {
    float translateX;
    float translateY;
    float rotateRate;
    float scaleXRate;
    float scaleYRate;
    float shearXRate;
    float shearYRate;
};

struct ObjectTransform3DStateEvent {
    float translateX;
    float translateY;
    float translateZ;
    float rotateXRate;
    float rotateYRate;
    float rotateZRate;
    float scaleXRate;
    float scaleYRate;
    float scaleZRate;
};

struct CameraMoveInputEvent {
    float forward;
    float right;
    float up;
};

struct SceneSwitchRequest {
    ESceneType type;
};

struct ToggleCameraModeRequest {
};

class ITransform2DInputSink {
public:
    virtual ~ITransform2DInputSink() = default;

    virtual void OnTransformInputEvent(const TransformActionEvent& event) = 0;

    virtual void OnTransform2DStateEvent(const Transform2DStateEvent& event) = 0;
};

class ITransform3DInputSink {
public:
    virtual ~ITransform3DInputSink() = default;

    virtual void OnObjectTransform3DEvent(const ObjectTransform3DEvent& event) = 0;

    virtual void OnObjectTransform3DStateEvent(const ObjectTransform3DStateEvent& event) = 0;
};

class ICameraMoveInputSink {
public:
    virtual ~ICameraMoveInputSink() = default;

    virtual void OnCameraMoveInputEvent(const CameraMoveInputEvent& event) = 0;
};

class IScene {
public:
    virtual ~IScene() = default;

    virtual void Initialize(RenderContext& ctx) = 0;

    virtual void Update(float dt) = 0;

    virtual void Render(RenderContext& ctx, LegacyGuiRenderer& guiRenderer) = 0;

    virtual const char* Name() const = 0;

    virtual void RegisterInputHandlers(InputEventBus& eventBus) = 0;

    virtual void UnregisterInputHandlers(InputEventBus& eventBus) = 0;

};

#endif // BJTU_WGPU_RENDERER_SCENE_H
