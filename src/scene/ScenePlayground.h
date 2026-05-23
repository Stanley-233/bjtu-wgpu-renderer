#ifndef BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
#define BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H

#include <glm/mat4x4.hpp>

#include "scene/Entity.h"
#include "scene/LogicScene.h"

class ScenePlayground final : public LogicScene {
public:
    [[nodiscard]] const char* Name() const override;

    void Update(float dt) override;

    [[nodiscard]] bool IsMagentaPointLightEnabled() const;

    [[nodiscard]] bool IsBluePointLightEnabled() const;

    void SetMagentaPointLightEnabled(bool enabled);

    void SetBluePointLightEnabled(bool enabled);

protected:
    [[nodiscard]] bool BuildSceneContent() override;

    void ConfigureInitialCamera(CameraComponent& camera) override;

    void ConfigureInitialDirectionalLight(DirectionalLightComponent& light) override;

private:
    void ApplyPointLightEnabledStates();

    Entity    m_magentaPointLight{};
    Entity    m_bluePointLight{};
    bool      m_magentaPointLightEnabled = true;
    bool      m_bluePointLightEnabled = true;
};

#endif // BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
