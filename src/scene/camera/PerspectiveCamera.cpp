#include "PerspectiveCamera.h"

#include <cmath>
#include <glm/ext/matrix_transform.hpp>

void PerspectiveCamera::SetPerspective(const float fovYRadians, const float nearPlane, const float farPlane) {
    m_fovYRadians = fovYRadians;
    m_nearPlane   = nearPlane;
    m_farPlane    = farPlane;
}

void PerspectiveCamera::SetFovYRadians(const float fovYRadians) {
    m_fovYRadians = fovYRadians;
}

float PerspectiveCamera::FovYRadians() const {
    return m_fovYRadians;
}

glm::mat4 PerspectiveCamera::View() const {
    return glm::lookAtRH(m_position, m_target, m_up);
}

glm::mat4 PerspectiveCamera::Projection(const float aspect) const {
    // 透视投影矩阵（WebGPU 深度范围 [0, 1]）
    const float safeAspect = aspect > 0.0f ? aspect : 1.0f;
    const float safeNear   = m_nearPlane > 0.0f ? m_nearPlane : 0.1f;
    const float safeFar    = m_farPlane > safeNear ? m_farPlane : (safeNear + 100.0f);

    const float tanHalfFovy = std::tan(m_fovYRadians * 0.5f);
    const float f           = 1.0f / tanHalfFovy;
    const float zScale      = safeFar / (safeNear - safeFar);
    const float zTranslate  = (safeFar * safeNear) / (safeNear - safeFar);

    return glm::mat4(
        glm::vec4(f / safeAspect, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, zScale, -1.0f),
        glm::vec4(0.0f, 0.0f, zTranslate, 0.0f)
    );
}
