#ifndef BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
#define BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H

#include "scene/LogicScene.h"

class ScenePlayground final : public LogicScene {
public:
    [[nodiscard]] const char* Name() const override;

protected:
    [[nodiscard]] bool BuildSceneContent() override;

    void ConfigureInitialCamera(CameraComponent& camera) override;

    void ConfigureInitialDirectionalLight(DirectionalLightComponent& light) override;
};

#endif // BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
