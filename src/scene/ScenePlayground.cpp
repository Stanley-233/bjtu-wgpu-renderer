#include "ScenePlayground.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "math/Transform3D.h"

namespace {
constexpr float kArkZfyBaseRotationX = -3.14f / 2.0f;
constexpr glm::vec3 kArkZfyTranslation{1.5f, 0.0f, 0.0f};
constexpr glm::vec3 kFanTranslation{0.2f, 3.0f, -1.8f};
constexpr glm::vec3 kFanScale{0.05f, 0.05f, 0.05f};
constexpr float kFanRotationSpeed = 12.0f;
}

const char* ScenePlayground::Name() const {
    return "ScenePlayground";
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
    // 风扇要转的部分
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
