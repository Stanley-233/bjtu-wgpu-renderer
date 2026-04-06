#include "PerspectiveCamera.h"

void PerspectiveCamera::SetPerspective(const float fovYRadians, const float nearPlane, const float farPlane) {
    m_fovYRadians = fovYRadians;
    m_nearPlane   = nearPlane;
    m_farPlane    = farPlane;
}

glm::mat4 PerspectiveCamera::View() const {
    // TODO: 根据相机位置/目标/up 计算并返回透视相机 View 矩阵
    const glm::vec3 f = glm::normalize(m_target - m_position);
    const glm::vec3 s = glm::normalize(glm::cross(f, m_up));
    const glm::vec3 u = glm::cross(s, f); 
    return glm::mat4(
        glm::vec4(s,0),
        glm::vec4(u,0),
        glm::vec4(-f,0),
        glm::vec4(-glm::dot(s, m_position), -glm::dot(u, m_position), glm::dot(f, m_position), 1.0f)
    );
}

glm::mat4 PerspectiveCamera::Projection(const float aspect) const {
    // TODO: 根据 fov、near、far、aspect 计算透视投影矩阵
    const float t = 1.0f / std::tan( m_fovYRadians * 0.5f);
    const float temp = m_farPlane / (m_nearPlane - m_farPlane);
    return glm::mat4(
        glm::vec4(t / aspect, 0, 0, 0),
        glm::vec4(0, -t, 0, 0),
        glm::vec4(0, 0, temp, -m_nearPlane * temp),
        glm::vec4(0, 0, 1.0f, 0)
    );
}
