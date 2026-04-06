#include "Scene3D.h"

#include <memory>

#include "../../resource/ResourceManager.h"
#include "../../resource/models/SceneDescription.h"
#include "../camera/OrthographicCamera.h"
#include "../camera/PerspectiveCamera.h"

void Scene3D::Initialize(RenderContext& ctx) {
    m_renderer.Initialize(ctx);
    SceneDescription sceneDescription{};
    if (!ResourceManager::LoadSceneFromToml(RESOURCE_DIR "/scene3d.toml", sceneDescription)) {
        std::cerr << "[Scene3D] Failed to load scene description from " RESOURCE_DIR "/scene3d.toml" << std::endl;
        m_objects.clear();
        return;
    }
    if (sceneDescription.camera.projectionType == ECameraProjectionType::Orthographic) {
        SetCameraMode(ECameraMode::Orthographic);
    } else {
        SetCameraMode(ECameraMode::Perspective);
    }
    if (m_camera != nullptr) {
        m_camera->SetPose(
            sceneDescription.camera.position,
            sceneDescription.camera.target,
            sceneDescription.camera.up
        );
    }
    m_objects.clear();
    m_objects.reserve(sceneDescription.objects.size());
    for (const ObjectDescription& objectDescription : sceneDescription.objects) {
        m_objects.emplace_back();
        m_objects.back().SetMesh(objectDescription.mesh);
    }
}

void Scene3D::Update(float dt) {
    // TODO: 实现 3D 逐帧更新逻辑
    (void)dt;
}

void Scene3D::Render(RenderContext& ctx) {
    if (m_camera == nullptr) {
        return;
    }
    m_renderer.SyncScene(ctx, m_objects, *m_camera);
    m_renderer.RenderFrame(ctx);
}

const char* Scene3D::Name() const {
    return "Scene3D";
}

// TODO: 对该场景控制的所有物体进行变换操作
void Scene3D::OnTransformAction(const ETransformAction action, const float amountX, const float amountY) {
    (void)action;
    (void)amountX;
    (void)amountY;
}

void Scene3D::ToggleCameraMode() {
    if (m_cameraMode == ECameraMode::Perspective) {
        SetCameraMode(ECameraMode::Orthographic);
        return;
    }
    SetCameraMode(ECameraMode::Perspective);
}

Scene3D::ECameraMode Scene3D::CameraMode() const {
    return m_cameraMode;
}

void Scene3D::SetCameraMode(const ECameraMode mode) {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 target   = {0.0f, 0.0f, -1.0f};
    glm::vec3 up       = {0.0f, 1.0f, 0.0f};
    if (m_camera != nullptr) {
        position = m_camera->Position();
        target   = m_camera->Target();
        up       = m_camera->Up();
    }
    m_cameraMode = mode;
    if (mode == ECameraMode::Perspective) {
        m_camera = std::make_unique<PerspectiveCamera>();
    } else {
        m_camera = std::make_unique<OrthographicCamera>();
    }
    m_camera->SetPose(position, target, up);
}
