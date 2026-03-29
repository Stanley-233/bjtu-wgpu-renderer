#ifndef BJTU_WGPU_RENDERER_SCENE_H
#define BJTU_WGPU_RENDERER_SCENE_H

class RenderContext;

enum class ESceneType {
    Scene2D,
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

class IScene {
public:
    virtual ~IScene() = default;

    virtual void Initialize(RenderContext& ctx) = 0;

    virtual void Update(float dt) = 0;

    virtual void Render(RenderContext& ctx) = 0;

    virtual const char* Name() const = 0;

    virtual void OnTransformAction(ETransformAction action, float amountX, float amountY) {
        (void)action;
        (void)amountX;
        (void)amountY;
    }

    virtual void OnTransformInputEvent(const TransformActionEvent& event) {
        OnTransformAction(event.action, event.amountX, event.amountY);
    }
};

#endif // BJTU_WGPU_RENDERER_SCENE_H
