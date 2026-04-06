#include "OrthographicCamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

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
    return glm::lookAtRH(m_position, m_target, m_up);
}

glm::mat4 OrthographicCamera::Projection(const float aspect) const {
    const float safeAspect = aspect > 0.0f ? aspect : 1.0f;
    const float centerX    = 0.5f * (m_left + m_right);
    const float halfWidth  = 0.5f * (m_right - m_left) * safeAspect;
    const float left       = centerX - halfWidth;
    const float right      = centerX + halfWidth;
    // WebGPU的NDC是[0,1]，不是opengl的[-1,1]
    return glm::orthoRH_ZO(left, right, m_bottom, m_top, m_nearPlane, m_farPlane);
}
