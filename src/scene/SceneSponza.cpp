#include "SceneSponza.h"

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/light/DirectionalLightComponent.h"

const char* SceneSponza::Name() const {
    return "SceneSponza";
}

bool SceneSponza::BuildSceneContent() {
    const Entity root = LoadModelRoot("Sponza/glTF/Sponza.gltf", "Sponza");
    return root.IsValid();
}

void SceneSponza::ConfigureInitialCamera(CameraComponent& camera) {
    if (camera.camera != nullptr) {
        camera.camera->SetPose(
            glm::vec3{0.0f, 3.0f, 8.0f},
            glm::vec3{0.0f, 3.0f, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f});
    }
}

void SceneSponza::ConfigureInitialDirectionalLight(DirectionalLightComponent& light) {
    light.direction = glm::vec3{-0.25f, -1.0f, -0.35f};
    light.intensity = 2.2f;
    light.color = glm::vec3{1.0f, 0.98f, 0.95f};
}
