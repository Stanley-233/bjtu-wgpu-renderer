#include "Transform2D.h"
#include <cmath>

Transform2D Transform2D::Identity() {
    return Transform2D{};
}

// construct a translation matrix from tx/ty.
Transform2D Transform2D::Translation(float tx, float ty) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f, 0.0f, tx,
        0.0f, 1.0f, ty,
        0.0f, 0.0f, 1.0f
    );
    return transform;
}

// construct a rotation matrix around the 2D origin.
Transform2D Transform2D::Rotation(float radians) {
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        cos_theta, -sin_theta, 0.0f,
        sin_theta,  cos_theta, 0.0f,
        0.0f,       0.0f,      1.0f
    );
    return transform;
}

// construct a non-uniform scale matrix.
Transform2D Transform2D::Scale(float sx, float sy) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        sx, 0.0f, 0.0f,
        0.0f, sy, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    return transform;
}

// construct a shear matrix with x/y shear factors.
Transform2D Transform2D::Shear(float shx, float shy) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f, shx, 0.0f,
        shy, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    return transform;
}

// return matrix for reflection across X axis.
Transform2D Transform2D::ReflectionX() {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f,  0.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f,  0.0f, 1.0f
    );
    return transform;
}

// return matrix for reflection across Y axis.
Transform2D Transform2D::ReflectionY() {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        -1.0f, 0.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
        0.0f,  0.0f, 1.0f
    );
    return transform;
}

// define and apply transform composition order.
Transform2D& Transform2D::Combine(const Transform2D& rhs) {
    // 矩阵乘法，注意顺序：this = this * rhs
    m_matrix = m_matrix * rhs.m_matrix;
    return *this;
}

const glm::mat3& Transform2D::Matrix() const {
    return m_matrix;
}

void Transform2D::Reset() {
    m_matrix = glm::mat3(1.0f);
}
