#include "FreeCameraController.h"

#include <cmath>
#include <glm/geometric.hpp>

#include "scene/camera/Camera.h"

void FreeCameraController::OnMoveInput(const CameraMoveInputEvent& event) {
    m_moveForward = event.forward;
    m_moveRight = event.right;
    m_moveUp = event.up;
}

void FreeCameraController::OnLookInput(const CameraLookInputEvent& event) {
    m_yaw += event.deltaYaw * kLookSensitivity;
    m_pitch += event.deltaPitch * kLookSensitivity;
    if (m_pitch < -1.5f) m_pitch = -1.5f;
    if (m_pitch > 1.5f) m_pitch = 1.5f;
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

    // 平移方向依赖当前朝向
    const glm::vec3 forward = forwardRaw / forwardLen;
    glm::vec3 rightRaw = glm::cross(forward, up);
    const float rightLen = glm::length(rightRaw);
    if (rightLen <= kEpsilon) {
        return;
    }
    rightRaw /= rightLen;

    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // -- 平移 --
    glm::vec3 newPos = position;
    glm::vec3 moveDirection = forward * m_moveForward + rightRaw * m_moveRight + worldUp * m_moveUp;
    const float moveDirectionLen = glm::length(moveDirection);
    if (moveDirectionLen > kEpsilon) {
        newPos = position + (moveDirection / moveDirectionLen) * (kMoveSpeed * dt);
    }

    // -- 旋转 --
    glm::vec3 lookDir;
    lookDir.x = std::sin(m_yaw) * std::cos(m_pitch);
    lookDir.y = std::sin(m_pitch);
    lookDir.z = -std::cos(m_yaw) * std::cos(m_pitch);

    camera.SetPose(newPos, newPos + lookDir * forwardLen, worldUp);
}