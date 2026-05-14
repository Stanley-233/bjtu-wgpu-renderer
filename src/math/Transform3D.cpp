#include "Transform3D.h"

#include <cmath>

Transform3D Transform3D::Identity() {
    return Transform3D{};
}

// 按列向量约定构造3D平移矩阵：p' = M * p
Transform3D Transform3D::Translation(float tx, float ty, float tz) {
    Transform3D transform;
    transform.m_matrix = glm::mat4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        tx,   ty,   tz,   1.0f
    );
    return transform;
}

// 构造绕 X 轴旋转矩阵（右手坐标系，顺时针旋转）
Transform3D Transform3D::RotationX(float radians) {
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    Transform3D transform;
    transform.m_matrix = glm::mat4(
        1.0f, 0.0f,      0.0f,     0.0f,
        0.0f, cos_theta, -sin_theta, 0.0f,
        0.0f, sin_theta, cos_theta, 0.0f,
        0.0f, 0.0f,      0.0f,     1.0f
    );
    return transform;
}

// 构造绕 Y 轴旋转矩阵（右手坐标系，顺时针旋转）
Transform3D Transform3D::RotationY(float radians) {
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    Transform3D transform;
    transform.m_matrix = glm::mat4(
        cos_theta, 0.0f, sin_theta, 0.0f,
        0.0f,      1.0f, 0.0f,      0.0f,
        -sin_theta, 0.0f, cos_theta, 0.0f,
        0.0f,      0.0f, 0.0f,      1.0f
    );
    return transform;
}

// 构造绕 Z 轴旋转矩阵（右手坐标系，顺时针旋转）
Transform3D Transform3D::RotationZ(float radians) {
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    Transform3D transform;
    transform.m_matrix = glm::mat4(
        cos_theta, -sin_theta, 0.0f, 0.0f,
        sin_theta, cos_theta,  0.0f, 0.0f,
        0.0f,      0.0f,       1.0f, 0.0f,
        0.0f,      0.0f,       0.0f, 1.0f
    );
    return transform;
}

// 构造3D非均匀缩放矩阵
Transform3D Transform3D::Scale(float sx, float sy, float sz) {
    Transform3D transform;
    transform.m_matrix = glm::mat4(
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, sz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    return transform;
}

Transform3D& Transform3D::Combine(const Transform3D& rhs) {
    m_matrix = rhs.m_matrix * m_matrix;
    return *this;
}

const glm::mat4& Transform3D::Matrix() const {
    return m_matrix;
}

void Transform3D::SetMatrix(const glm::mat4& matrix) {
    m_matrix = matrix;
}

void Transform3D::Reset() {
    m_matrix = glm::mat4(1.0f);
}
