#include "SceneSponza.h"

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/light/DirectionalLightComponent.h"

const char* SceneSponza::Name() const {
    return "SceneSponza";
}

bool SceneSponza::BuildSceneContent() {
    ModelSpawnOptions options{};
    options.skipMaskedMaterials = true;
    const Entity root = LoadModelRoot("Sponza/glTF/Sponza.gltf", "Sponza", options);
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
    light.direction = glm::normalize(glm::vec3{-0.45f, -0.75f, 0.45f});
    light.intensity = 2.5f;
    light.color = glm::vec3{1.0f, 0.96f, 0.90f};
}
