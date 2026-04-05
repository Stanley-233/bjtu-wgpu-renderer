#include "Scene3D.h"

#include <memory>

#include "../camera/OrthographicCamera.h"
#include "../camera/PerspectiveCamera.h"
#include "../../render/RenderContext.h"

void Scene3D::Initialize(RenderContext& ctx) {
    m_renderer.Initialize(ctx);
    SetCameraMode(ECameraMode::Perspective);
}

void Scene3D::Update(float dt) {
    (void)dt;
}

void Scene3D::Render(RenderContext& ctx) {
    if (m_camera == nullptr) {
        return;
    }

    m_renderer.SyncScene(m_objects, *m_camera);
    m_renderer.RenderFrame(ctx);
}

const char* Scene3D::Name() const {
    return "Scene3D";
}

void Scene3D::OnTransformAction(const ETransformAction action, const float amountX, const float amountY) {
    if (m_objects.empty()) {
        return;
    }

    Transform3D delta = Transform3D::Identity();
    switch (action) {
        case ETransformAction::Translate:
            delta = Transform3D::Translation(amountX, amountY, 0.0f);
            break;
        case ETransformAction::Rotate:
            delta = Transform3D::RotationZ(amountX);
            break;
        case ETransformAction::Scale:
            delta = Transform3D::Scale(amountX, amountY, 1.0f);
            break;
        case ETransformAction::Shear:
        case ETransformAction::ReflectX:
        case ETransformAction::ReflectY:
        case ETransformAction::Reset:
            break;
    }
    m_objects.front().Transform().Combine(delta);
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
    m_cameraMode = mode;
    if (mode == ECameraMode::Perspective) {
        m_camera = std::make_unique<PerspectiveCamera>();
        return;
    }
    m_camera = std::make_unique<OrthographicCamera>();
}
