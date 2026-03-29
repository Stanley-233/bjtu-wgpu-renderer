#include "Transform2D.h"

// TODO: decide whether Identity should return cached singleton or a value copy.
Transform2D Transform2D::Identity() {
    return Transform2D{};
}

// TODO: construct a translation matrix from tx/ty.
Transform2D Transform2D::Translation(float tx, float ty) {
    (void)tx;
    (void)ty;
    return Transform2D{};
}

// TODO: construct a rotation matrix around the 2D origin.
Transform2D Transform2D::Rotation(float radians) {
    (void)radians;
    return Transform2D{};
}

// TODO: construct a non-uniform scale matrix.
Transform2D Transform2D::Scale(float sx, float sy) {
    (void)sx;
    (void)sy;
    return Transform2D{};
}

// TODO: construct a shear matrix with x/y shear factors.
Transform2D Transform2D::Shear(float shx, float shy) {
    (void)shx;
    (void)shy;
    return Transform2D{};
}

// TODO: return matrix for reflection across X axis.
Transform2D Transform2D::ReflectionX() {
    return Transform2D{};
}

// TODO: return matrix for reflection across Y axis.
Transform2D Transform2D::ReflectionY() {
    return Transform2D{};
}

// TODO: define and apply transform composition order.
Transform2D& Transform2D::Combine(const Transform2D& rhs) {
    (void)rhs;
    return *this;
}

const glm::mat3& Transform2D::Matrix() const {
    return m_matrix;
}

void Transform2D::Reset() {
    m_matrix = glm::mat3(1.0f);
}
