#include "Transform2D.h"

#include <cmath>

Transform2D Transform2D::Identity() {
    return Transform2D{};
}

// 按列向量约定构造平移矩阵：p' = M * p
Transform2D Transform2D::Translation(float tx, float ty) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        tx,   ty,   1.0f
    );
    return transform;
}

// 构造绕二维原点逆时针旋转矩阵
Transform2D Transform2D::Rotation(float radians) {
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        cos_theta,  sin_theta, 0.0f,
        -sin_theta, cos_theta, 0.0f,
        0.0f,       0.0f,      1.0f
    );
    return transform;
}

// 构造非均匀缩放矩阵
Transform2D Transform2D::Scale(float sx, float sy) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        sx, 0.0f, 0.0f,
        0.0f, sy, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    return transform;
}

// 构造Shear矩阵，shx/shy 分别表示 x/y 方向Shear系数
Transform2D Transform2D::Shear(float shx, float shy) {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f, shy,  0.0f,
        shx,  1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    return transform;
}

// 构造关于 X 轴的反射矩阵
Transform2D Transform2D::ReflectionX() {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        1.0f,  0.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f,  0.0f, 1.0f
    );
    return transform;
}

// 构造关于 Y 轴的反射矩阵
Transform2D Transform2D::ReflectionY() {
    Transform2D transform;
    transform.m_matrix = glm::mat3(
        -1.0f, 0.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
        0.0f,  0.0f, 1.0f
    );
    return transform;
}

// 在列向量约定下按调用顺序叠加变换：M = rhs * M
Transform2D& Transform2D::Combine(const Transform2D& rhs) {
    m_matrix = rhs.m_matrix * m_matrix;
    return *this;
}

const glm::mat3& Transform2D::Matrix() const {
    return m_matrix;
}

Transform2D::Mat3Uniform Transform2D::ToWgslMat3Uniform() const {
    return ToWgslMat3Uniform(m_matrix);
}

Transform2D::Mat3Uniform Transform2D::ToWgslMat3Uniform(const glm::mat3& matrix) {
    // WGSL 的 uniform 地址空间中，mat3x3f 由 3 列组成，每列按 16 字节对齐
    return Mat3Uniform{
        {
            Mat3UniformColumn{matrix[0][0], matrix[0][1], matrix[0][2], 0.0f},
            Mat3UniformColumn{matrix[1][0], matrix[1][1], matrix[1][2], 0.0f},
            Mat3UniformColumn{matrix[2][0], matrix[2][1], matrix[2][2], 0.0f},
        },
    };
}

void Transform2D::Reset() {
    m_matrix = glm::mat3(1.0f);
}
