#include "SceneRoom.h"

#include <glm/geometric.hpp>

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "math/Transform3D.h"

const char* SceneRoom::Name() const {
    return "SceneRoom";
}

bool SceneRoom::BuildSceneContent() {
    const Entity root = LoadModelRoot("living_room_interior_free/scene.gltf", "Room");
    if (root.IsValid() && root.HasComponent<TransformComponent>()) {
        auto& [rootTransform] = root.GetComponent<TransformComponent>();
        rootTransform = Transform3D::Scale(0.2f, 0.2f, 0.2f);
    }
    return root.IsValid();
}

void SceneRoom::ConfigureInitialCamera(CameraComponent& camera) {
    if (camera.camera != nullptr) {
        camera.camera->SetPose(
            glm::vec3{0.0f, 1.6f, 4.0f},
            glm::vec3{0.0f, 1.2f, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f});
    }
}

void SceneRoom::ConfigureInitialDirectionalLight(DirectionalLightComponent& light) {
    light.direction = glm::normalize(glm::vec3{0.769f, -0.572f, 0.285f});
    light.intensity = 1.5f;
    light.color = glm::vec3{1.0f, 0.97f, 0.92f};
}
