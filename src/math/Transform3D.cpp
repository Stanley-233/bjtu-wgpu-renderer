#include "Transform3D.h"

Transform3D Transform3D::Identity() {
    return Transform3D{};
}

// TODO: 平移变换
Transform3D Transform3D::Translation(float tx, float ty) {
    (void)tx;
    (void)ty;
    return Transform3D{};
}

// TODO: 旋转变换
Transform3D Transform3D::Rotation(float radians) {
    (void)radians;
    return Transform3D{};
}

// TODO: 缩放变换
Transform3D Transform3D::Scale(float sx, float sy) {
    (void)sx;
    (void)sy;
    return Transform3D{};
}

// TODO: 切变变换
Transform3D Transform3D::Shear(float shx, float shy) {
    (void)shx;
    (void)shy;
    return Transform3D{};
}

// TODO: 反转变换
Transform3D Transform3D::Reflection(EReflectionType type) {
    (void)type;
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
