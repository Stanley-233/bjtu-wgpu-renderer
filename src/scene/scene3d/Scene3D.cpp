#include "Scene3D.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
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
        m_initialObjectTransforms.clear();
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
    m_initialObjectTransforms.clear();
    m_objects.reserve(sceneDescription.objects.size());
    m_initialObjectTransforms.reserve(sceneDescription.objects.size());
    for (const ObjectDescription& objectDescription : sceneDescription.objects) {
        m_objects.emplace_back();
        Object3D& object = m_objects.back();
        object.SetMesh(objectDescription.mesh);

        Transform3D initialTransform = Transform3D::Identity();
        initialTransform.Combine(Transform3D::Scale(
            objectDescription.scale.x,
            objectDescription.scale.y,
            objectDescription.scale.z
        ));
        initialTransform.Combine(Transform3D::RotationX(objectDescription.rotation.x));
        initialTransform.Combine(Transform3D::RotationY(objectDescription.rotation.y));
        initialTransform.Combine(Transform3D::RotationZ(objectDescription.rotation.z));
        initialTransform.Combine(Transform3D::Translation(
            objectDescription.translation.x,
            objectDescription.translation.y,
            objectDescription.translation.z
        ));
        object.SetTransform(initialTransform);
        m_initialObjectTransforms.push_back(initialTransform);
    }
}

void Scene3D::Update(float dt) {
    constexpr float kEpsilon = 1e-6f;
    if (dt <= 0.0f) {
        return;
    }

    if (m_camera != nullptr) {
        const glm::vec3 position = m_camera->Position();
        const glm::vec3 target   = m_camera->Target();
        const glm::vec3 up       = m_camera->Up();

        const glm::vec3 forwardRaw = target - position;
        const float     forwardLen = glm::length(forwardRaw);
        if (forwardLen > 1e-6f) {
            const glm::vec3 forward = forwardRaw / forwardLen;

            glm::vec3 rightRaw = glm::cross(forward, up);
            const float rightLen = glm::length(rightRaw);
            if (rightLen > 1e-6f) {
                rightRaw /= rightLen;

                constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
                glm::vec3 moveDirection = forward * m_moveForward
                                          + rightRaw * m_moveRight
                                          + worldUp * m_moveUp;
                const float moveDirectionLen = glm::length(moveDirection);
                if (moveDirectionLen > 1e-6f) {
                    moveDirection /= moveDirectionLen;

                    const glm::vec3 delta = moveDirection * (kCameraMoveSpeed * dt);
                    m_camera->SetPose(position + delta, target + delta, up);
                }
            }
        }
    }

    const bool hasTranslate = std::fabs(m_objectTransformState.translateX) > kEpsilon
                              || std::fabs(m_objectTransformState.translateY) > kEpsilon
                              || std::fabs(m_objectTransformState.translateZ) > kEpsilon;
    const bool hasRotate = std::fabs(m_objectTransformState.rotateXRate) > kEpsilon
                           || std::fabs(m_objectTransformState.rotateYRate) > kEpsilon
                           || std::fabs(m_objectTransformState.rotateZRate) > kEpsilon;
    const bool hasScale = std::fabs(m_objectTransformState.scaleXRate) > kEpsilon
                          || std::fabs(m_objectTransformState.scaleYRate) > kEpsilon
                          || std::fabs(m_objectTransformState.scaleZRate) > kEpsilon;
    if (!hasTranslate && !hasRotate && !hasScale) {
        return;
    }

    for (Object3D& object : m_objects) {
        if (hasTranslate) {
            object.Transform().Combine(Transform3D::Translation(
                m_objectTransformState.translateX * dt,
                m_objectTransformState.translateY * dt,
                m_objectTransformState.translateZ * dt
            ));
        }
        if (hasRotate) {
            if (std::fabs(m_objectTransformState.rotateXRate) > kEpsilon) {
                object.Transform().Combine(Transform3D::RotationX(m_objectTransformState.rotateXRate * dt));
            }
            if (std::fabs(m_objectTransformState.rotateYRate) > kEpsilon) {
                object.Transform().Combine(Transform3D::RotationY(m_objectTransformState.rotateYRate * dt));
            }
            if (std::fabs(m_objectTransformState.rotateZRate) > kEpsilon) {
                object.Transform().Combine(Transform3D::RotationZ(m_objectTransformState.rotateZRate * dt));
            }
        }
        if (hasScale) {
            const float sx = std::exp(m_objectTransformState.scaleXRate * dt);
            const float sy = std::exp(m_objectTransformState.scaleYRate * dt);
            const float sz = std::exp(m_objectTransformState.scaleZRate * dt);
            object.Transform().Combine(Transform3D::Scale(sx, sy, sz));
        }
    }
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

void Scene3D::OnObjectTransform3DEvent(const ObjectTransform3DEvent& event) {
    if (event.mode != EObjectTransform3DMode::Reset) {
        return;
    }
    const size_t count = std::min(m_objects.size(), m_initialObjectTransforms.size());
    for (size_t i = 0; i < count; ++i) {
        m_objects[i].SetTransform(m_initialObjectTransforms[i]);
    }
    for (size_t i = count; i < m_objects.size(); ++i) {
        m_objects[i].Transform().Reset();
    }
}

void Scene3D::OnObjectTransform3DStateEvent(const ObjectTransform3DStateEvent& event) {
    m_objectTransformState = event;
}

void Scene3D::OnCameraMoveInputEvent(const CameraMoveInputEvent& event) {
    m_moveForward = event.forward;
    m_moveRight   = event.right;
    m_moveUp      = event.up;
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
