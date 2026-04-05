#include "PerspectiveCamera.h"

void PerspectiveCamera::SetPerspective(const float fovYRadians, const float nearPlane, const float farPlane) {
    m_fovYRadians = fovYRadians;
    m_nearPlane   = nearPlane;
    m_farPlane    = farPlane;
}

glm::mat4 PerspectiveCamera::View() const {
    // TODO: 根据相机位置/目标/up 计算并返回透视相机 View 矩阵
    return glm::mat4(1.0f);
}

glm::mat4 PerspectiveCamera::Projection(const float aspect) const {
    // TODO: 根据 fov、near、far、aspect 计算透视投影矩阵
    (void)aspect;
    return glm::mat4(1.0f);
}
