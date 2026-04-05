#include "Transform3D.h"

Transform3D Transform3D::Identity() {
    return Transform3D{};
}

Transform3D Transform3D::Translation(float tx, float ty, float tz) {
    (void)tx;
    (void)ty;
    (void)tz;
    return Transform3D{};
}

Transform3D Transform3D::RotationX(float radians) {
    (void)radians;
    return Transform3D{};
}

Transform3D Transform3D::RotationY(float radians) {
    (void)radians;
    return Transform3D{};
}

Transform3D Transform3D::RotationZ(float radians) {
    (void)radians;
    return Transform3D{};
}

Transform3D Transform3D::Scale(float sx, float sy, float sz) {
    (void)sx;
    (void)sy;
    (void)sz;
    return Transform3D{};
}

Transform3D& Transform3D::Combine(const Transform3D& rhs) {
    m_matrix = rhs.m_matrix * m_matrix;
    return *this;
}

const glm::mat4& Transform3D::Matrix() const {
    return m_matrix;
}

void Transform3D::Reset() {
    m_matrix = glm::mat4(1.0f);
}
