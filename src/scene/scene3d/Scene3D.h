#ifndef BJTU_WGPU_RENDERER_SCENE3D_H
#define BJTU_WGPU_RENDERER_SCENE3D_H

#include <memory>
#include <vector>

#include "../../render/Renderer3D.h"
#include "../Scene.h"
#include "../camera/Camera.h"
#include "Object3D.h"

class Scene3D final : public IScene {
public:
    enum class ECameraMode {
        Perspective,
        Orthographic,
    };

    void Initialize(RenderContext& ctx) override;

    void Update(float dt) override;

    void Render(RenderContext& ctx) override;

    const char* Name() const override;

    void OnTransformAction(ETransformAction action, float amountX, float amountY) override;

    void ToggleCameraMode();

    [[nodiscard]] ECameraMode CameraMode() const;

private:
    void SetCameraMode(ECameraMode mode);

    ECameraMode             m_cameraMode = ECameraMode::Perspective;
    std::unique_ptr<Camera> m_camera{};
    std::vector<Object3D>   m_objects{};
    Renderer3D              m_renderer{};
};

#endif // BJTU_WGPU_RENDERER_SCENE3D_H
