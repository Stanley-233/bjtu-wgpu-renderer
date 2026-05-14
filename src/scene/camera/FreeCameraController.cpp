#include "FreeCameraController.h"

#include <glm/geometric.hpp>

#include "scene/camera/Camera.h"

void FreeCameraController::OnMoveInput(const CameraMoveInputEvent& event) {
    m_moveForward = event.forward;
    m_moveRight = event.right;
    m_moveUp = event.up;
}

void FreeCameraController::OnLookInput(const CameraLookInputEvent& event) {
    (void)event;
    // TODO：后续在这里应用鼠标驱动的 yaw/pitch 更新。
}

void FreeCameraController::Update(const float dt, Camera& camera) {
    constexpr float kEpsilon = 1e-6f;
    if (dt <= 0.0f) {
        return;
    }

    const glm::vec3 position = camera.Position();
    const glm::vec3 target = camera.Target();
    const glm::vec3 up = camera.Up();
    const glm::vec3 forwardRaw = target - position;
    const float forwardLen = glm::length(forwardRaw);
    if (forwardLen <= kEpsilon) {
        return;
    }

    const glm::vec3 forward = forwardRaw / forwardLen;
    glm::vec3 rightRaw = glm::cross(forward, up);
    const float rightLen = glm::length(rightRaw);
    if (rightLen <= kEpsilon) {
        return;
    }
    rightRaw /= rightLen;

    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    glm::vec3 moveDirection = forward * m_moveForward
                              + rightRaw * m_moveRight
                              + worldUp * m_moveUp;
    const float moveDirectionLen = glm::length(moveDirection);
    if (moveDirectionLen <= kEpsilon) {
        return;
    }

    moveDirection /= moveDirectionLen;
    const glm::vec3 delta = moveDirection * (kMoveSpeed * dt);
    camera.SetPose(position + delta, target + delta, up);
}
