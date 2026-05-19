#include "ScenePlayground.h"

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/TransformComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "math/Transform3D.h"

const char* ScenePlayground::Name() const {
    return "ScenePlayground";
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

    auto& [arkTransform] = arkZfyRoot.GetComponent<TransformComponent>();
    arkTransform.SetTranslation(1.5f, 0.0f, 0.0f);
    arkTransform = Transform3D::RotationX(-3.14f / 2.0f);

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
