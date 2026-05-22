#ifndef BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
#define BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H

#include <glm/mat4x4.hpp>

#include "scene/Entity.h"
#include "scene/LogicScene.h"

class ScenePlayground final : public LogicScene {
public:
    [[nodiscard]] const char* Name() const override;

    void Update(float dt) override;

protected:
    [[nodiscard]] bool BuildSceneContent() override;

    void ConfigureInitialCamera(CameraComponent& camera) override;

    void ConfigureInitialDirectionalLight(DirectionalLightComponent& light) override;

private:
    Entity    m_fanRotor{};
    glm::mat4 m_fanRotorBaseMatrix{1.0f};
    float     m_fanRotationRadians = 0.0f;
};

#endif // BJTU_WGPU_RENDERER_SCENEPLAYGROUND_H
