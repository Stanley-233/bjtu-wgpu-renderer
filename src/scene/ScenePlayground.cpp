#include "ScenePlayground.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "components/light/PointLightComponent.h"
#include "math/Transform3D.h"

namespace {
constexpr float kArkZfyBaseRotationX = -3.14f / 2.0f;
constexpr glm::vec3 kArkZfyTranslation{1.5f, 0.0f, 0.0f};
constexpr glm::vec3 kFanTranslation{0.2f, 3.0f, -1.8f};
constexpr glm::vec3 kFanScale{0.05f, 0.05f, 0.05f};
constexpr float kFanRotationSpeed = 12.0f;
constexpr glm::vec3 kCornellPointLightLeftTranslation{-1.95f, 1.08f, -2.18f};
constexpr glm::vec3 kCornellPointLightRightTranslation{-1.22f, 1.08f, -2.35f};
constexpr glm::vec3 kCornellPointLightLeftColor{1.0f, 0.15f, 0.75f};
constexpr glm::vec3 kCornellPointLightRightColor{0.2f, 0.45f, 1.0f};
constexpr float kCornellPointLightIntensity = 10.0f;
constexpr float kCornellPointLightRange = 4.5f;
}

const char* ScenePlayground::Name() const {
    return "ScenePlayground";
}

bool ScenePlayground::IsMagentaPointLightEnabled() const {
    return m_magentaPointLightEnabled;
}

bool ScenePlayground::IsBluePointLightEnabled() const {
    return m_bluePointLightEnabled;
}

void ScenePlayground::SetMagentaPointLightEnabled(const bool enabled) {
    m_magentaPointLightEnabled = enabled;
    ApplyPointLightEnabledStates();
}

void ScenePlayground::SetBluePointLightEnabled(const bool enabled) {
    m_bluePointLightEnabled = enabled;
    ApplyPointLightEnabledStates();
}

void ScenePlayground::Update(const float dt) {
    LogicScene::Update(dt);

    if (!m_fanRotor || !m_fanRotor.HasComponent<TransformComponent>()) {
        return;
    }

    m_fanRotationRadians += dt * kFanRotationSpeed;

    auto& transform = m_fanRotor.GetComponent<TransformComponent>().transform;
    transform.SetMatrix(
        m_fanRotorBaseMatrix
        * glm::rotate(glm::mat4(1.0f), m_fanRotationRadians, glm::vec3{0.0f, 1.0f, 0.0f}));
}

bool ScenePlayground::BuildSceneContent() {
    const ModelSpawnOptions arknightsOptions{
        .shadingModelOverride = EMaterialShadingModel::Unlit,
    };
    Entity arkZfyRoot = LoadModelRoot(
        "arknights/zhuang_fangyi__arknights_endfield.glb",
        "SimpleTexture",
        arknightsOptions);
    if (!arkZfyRoot) {
        return false;
    }

    auto& arkTransform = arkZfyRoot.GetComponent<TransformComponent>().transform;
    arkTransform = Transform3D::RotationX(kArkZfyBaseRotationX);
    arkTransform.Combine(Transform3D::Translation(
        kArkZfyTranslation.x,
        kArkZfyTranslation.y,
        kArkZfyTranslation.z));

    Entity fanRoot = LoadModelRoot("fan/scene.gltf", "fan");
    if (!fanRoot) {
        return false;
    }

    auto& fanRootTransform = fanRoot.GetComponent<TransformComponent>().transform;
    fanRootTransform = Transform3D::Scale(kFanScale.x, kFanScale.y, kFanScale.z);
    fanRootTransform.Combine(Transform3D::Translation(
        kFanTranslation.x,
        kFanTranslation.y,
        kFanTranslation.z));

    m_fanRotor = GetWorld().CreateEntity("fan rotor");
    auto& fanRotorTransform = m_fanRotor.AddComponent<TransformComponent>().transform;
    fanRotorTransform = Transform3D::Identity();
    m_fanRotor.SetParent(fanRoot);

    const auto fanMeshes = fanRoot.GetChildren();
    for (const Entity fanMesh : fanMeshes) {
        if (fanMesh == m_fanRotor) {
            continue;
        }
        fanMesh.SetParent(m_fanRotor);
    }
    m_fanRotorBaseMatrix = fanRotorTransform.Matrix();

    Entity cornelBoxRoot = LoadModelRoot("cornel-box-original/scene.gltf", "cornelBox");
    if (!cornelBoxRoot) {
        return false;
    }

    auto& [cornelBoxTransform] = cornelBoxRoot.GetComponent<TransformComponent>();
    cornelBoxTransform.SetTranslation(-1.5f, 0.0f, -3.0f);

    m_magentaPointLight = GetWorld().CreateEntity("playground magenta point light");
    auto& leftPointLightTransform = m_magentaPointLight.AddComponent<TransformComponent>().transform;
    leftPointLightTransform.SetTranslation(
        kCornellPointLightLeftTranslation.x,
        kCornellPointLightLeftTranslation.y,
        kCornellPointLightLeftTranslation.z);
    auto& leftPointLightComponent = m_magentaPointLight.AddComponent<PointLightComponent>();
    leftPointLightComponent.color = kCornellPointLightLeftColor;
    leftPointLightComponent.intensity = kCornellPointLightIntensity;
    leftPointLightComponent.range = kCornellPointLightRange;

    m_bluePointLight = GetWorld().CreateEntity("playground blue point light");
    auto& rightPointLightTransform = m_bluePointLight.AddComponent<TransformComponent>().transform;
    rightPointLightTransform.SetTranslation(
        kCornellPointLightRightTranslation.x,
        kCornellPointLightRightTranslation.y,
        kCornellPointLightRightTranslation.z);
    auto& rightPointLightComponent = m_bluePointLight.AddComponent<PointLightComponent>();
    rightPointLightComponent.color = kCornellPointLightRightColor;
    rightPointLightComponent.intensity = kCornellPointLightIntensity;
    rightPointLightComponent.range = kCornellPointLightRange;

    ApplyPointLightEnabledStates();

    return true;
}

void ScenePlayground::ConfigureInitialCamera(CameraComponent& camera) {
    if (camera.camera != nullptr) {
        camera.camera->SetPose(
            glm::vec3{0.0f, 0.0f, 3.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f});
    }
}

void ScenePlayground::ConfigureInitialDirectionalLight(DirectionalLightComponent& light) {
    light.direction = glm::vec3{-0.35f, -1.0f, -0.25f};
    light.intensity = 1.8f;
    light.color = glm::vec3{1.0f, 1.0f, 1.0f};
}

void ScenePlayground::ApplyPointLightEnabledStates() {
    if (m_magentaPointLight && m_magentaPointLight.HasComponent<PointLightComponent>()) {
        auto& pointLight = m_magentaPointLight.GetComponent<PointLightComponent>();
        pointLight.intensity = m_magentaPointLightEnabled ? kCornellPointLightIntensity : 0.0f;
    }
    if (m_bluePointLight && m_bluePointLight.HasComponent<PointLightComponent>()) {
        auto& pointLight = m_bluePointLight.GetComponent<PointLightComponent>();
        pointLight.intensity = m_bluePointLightEnabled ? kCornellPointLightIntensity : 0.0f;
    }
}
