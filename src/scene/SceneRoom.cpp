#include "SceneRoom.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "components/light/SpotLightComponent.h"
#include "math/Transform3D.h"

namespace {
constexpr glm::vec3 kRoomSpotLightTranslation{1.2f, 2.3f, -0.65f};
constexpr glm::vec3 kRoomSpotLightDirection{0.0f, -1.0f, 0.0f};
constexpr glm::vec3 kRoomSpotLightColor{1.0f, 0.84f, 0.68f};
constexpr float kRoomSpotLightIntensity = 8.0f;
constexpr float kRoomSpotLightRange = 4.8f;
constexpr float kRoomSpotLightInnerConeAngle = 0.28f;
constexpr float kRoomSpotLightOuterConeAngle = 0.46f;
} // namespace

const char* SceneRoom::Name() const {
    return "SceneRoom";
}

bool SceneRoom::IsSpotLightEnabled() const {
    return m_spotLightEnabled;
}

void SceneRoom::SetSpotLightEnabled(const bool enabled) {
    m_spotLightEnabled = enabled;
    ApplySpotLightEnabledState();
}

bool SceneRoom::BuildSceneContent() {
    const Entity root = LoadModelRoot("living_room_interior_free/scene.gltf", "Room");
    if (root.IsValid() && root.HasComponent<TransformComponent>()) {
        auto& [rootTransform] = root.GetComponent<TransformComponent>();
        rootTransform = Transform3D::Scale(0.2f, 0.2f, 0.2f);
    }
    if (!root.IsValid()) {
        return false;
    }

    m_spotLight = GetWorld().CreateEntity("room spotlight");
    auto& spotLightTransform = m_spotLight.AddComponent<TransformComponent>().transform;
    spotLightTransform.SetTranslation(
        kRoomSpotLightTranslation.x,
        kRoomSpotLightTranslation.y,
        kRoomSpotLightTranslation.z);

    auto& spotLightComponent = m_spotLight.AddComponent<SpotLightComponent>();
    spotLightComponent.direction = kRoomSpotLightDirection;
    spotLightComponent.color = kRoomSpotLightColor;
    spotLightComponent.intensity = kRoomSpotLightIntensity;
    spotLightComponent.range = kRoomSpotLightRange;
    spotLightComponent.innerConeAngle = kRoomSpotLightInnerConeAngle;
    spotLightComponent.outerConeAngle = kRoomSpotLightOuterConeAngle;
    ApplySpotLightEnabledState();

    return true;
}

void SceneRoom::ConfigureInitialCamera(CameraComponent& camera) {
    if (camera.camera != nullptr) {
        camera.camera->SetPose(
            glm::vec3{3.6f, 1.6f, 1.78f},
            glm::vec3{1.05f, 1.18f, -1.30f},
            glm::vec3{0.0f, 1.0f, 0.0f});
    }
}

void SceneRoom::ConfigureInitialDirectionalLight(DirectionalLightComponent& light) {
    light.direction = glm::normalize(glm::vec3{0.769f, -0.572f, 0.285f});
    light.intensity = 1.5f;
    light.color = glm::vec3{1.0f, 0.97f, 0.92f};
}

void SceneRoom::ApplySpotLightEnabledState() {
    if (m_spotLight && m_spotLight.HasComponent<SpotLightComponent>()) {
        auto& spotLight = m_spotLight.GetComponent<SpotLightComponent>();
        spotLight.intensity = m_spotLightEnabled ? kRoomSpotLightIntensity : 0.0f;
    }
}
