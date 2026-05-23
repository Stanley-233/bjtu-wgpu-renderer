#ifndef BJTU_WGPU_RENDERER_SCENEROOM_H
#define BJTU_WGPU_RENDERER_SCENEROOM_H

#include "Entity.h"
#include "scene/LogicScene.h"

class SceneRoom final : public LogicScene {
public:
    [[nodiscard]] const char* Name() const override;

    [[nodiscard]] bool IsSpotLightEnabled() const;

    void SetSpotLightEnabled(bool enabled);

protected:
    [[nodiscard]] bool BuildSceneContent() override;

    void ConfigureInitialCamera(CameraComponent& camera) override;

    void ConfigureInitialDirectionalLight(DirectionalLightComponent& light) override;

private:
    void ApplySpotLightEnabledState();

    Entity m_spotLight{};
    bool   m_spotLightEnabled = true;
};

#endif // BJTU_WGPU_RENDERER_SCENEROOM_H
