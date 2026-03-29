#ifndef BJTU_WGPU_RENDERER_SCENE_H
#define BJTU_WGPU_RENDERER_SCENE_H

class RenderContext;

enum class ESceneType {
    Scene2D,
};

class IScene {
public:
    virtual ~IScene() = default;

    virtual void Initialize(RenderContext& ctx) = 0;

    virtual void Update(float dt) = 0;

    virtual void Render(RenderContext& ctx) = 0;

    virtual const char* Name() const = 0;
};

#endif // BJTU_WGPU_RENDERER_SCENE_H