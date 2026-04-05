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
    // TODO: 根据相机位置/目标/up 计算并返回正交相机 View 矩阵
    return glm::mat4(1.0f);
}

glm::mat4 OrthographicCamera::Projection(const float aspect) const {
    // TODO: 按 left/right/top/bottom/near/far 计算正交投影矩阵（aspect 当前未使用）
    (void)aspect;
    return glm::mat4(1.0f);
}
