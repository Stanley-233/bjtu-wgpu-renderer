#include "PerspectiveCamera.h"

void PerspectiveCamera::SetPerspective(const float fovYRadians, const float nearPlane, const float farPlane) {
    m_fovYRadians = fovYRadians;
    m_nearPlane   = nearPlane;
    m_farPlane    = farPlane;
}

glm::mat4 PerspectiveCamera::View() const {
    return glm::mat4(1.0f);
}

glm::mat4 PerspectiveCamera::Projection(const float aspect) const {
    (void)aspect;
    return glm::mat4(1.0f);
}
