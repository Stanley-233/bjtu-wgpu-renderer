#include "Camera.h"

void Camera::SetPose(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) {
    m_position = position;
    m_target   = target;
    m_up       = up;
}

const glm::vec3& Camera::Position() const {
    return m_position;
}

const glm::vec3& Camera::Target() const {
    return m_target;
}

const glm::vec3& Camera::Up() const {
    return m_up;
}
