#include "OrthographicCamera.h"

void OrthographicCamera::SetOrthographic(
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float nearPlane,
    const float farPlane
) {
    m_left      = left;
    m_right     = right;
    m_bottom    = bottom;
    m_top       = top;
    m_nearPlane = nearPlane;
    m_farPlane  = farPlane;
}

glm::mat4 OrthographicCamera::View() const {
    return glm::mat4(1.0f);
}

glm::mat4 OrthographicCamera::Projection(const float aspect) const {
    (void)aspect;
    return glm::mat4(1.0f);
}
