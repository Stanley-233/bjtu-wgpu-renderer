#ifndef BJTU_WGPU_RENDERER_SCENEROOM_H
#define BJTU_WGPU_RENDERER_SCENEROOM_H

#include "Entity.h"
#include "scene/LogicScene.h"
#include <glm/mat4x4.hpp>

class SceneRoom final : public LogicScene {
public:
    [[nodiscard]] const char* Name() const override;

    void Update(float dt) override;

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

    Entity    m_fanRotor{};
    glm::mat4 m_fanRotorBaseMatrix{1.0f};
    float     m_fanRotationRadians = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_SCENEROOM_H
