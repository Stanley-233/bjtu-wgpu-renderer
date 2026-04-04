#ifndef BJTU_WGPU_RENDERER_TRANSFORM2D_H
#define BJTU_WGPU_RENDERER_TRANSFORM2D_H

#include <cstdint>
#include <glm/mat3x3.hpp>

class Transform2D {
public:
    struct Mat3UniformColumn {
        float x;
        float y;
        float z;
        float padding;
    };

    struct Mat3Uniform {
        Mat3UniformColumn columns[3];
    };

    static constexpr uint64_t kMat3UniformSize = sizeof(Mat3Uniform);

    static Transform2D Identity();

    static Transform2D Translation(float tx, float ty);

    static Transform2D Rotation(float radians);

    static Transform2D Scale(float sx, float sy);

    static Transform2D Shear(float shx, float shy);

    static Transform2D ReflectionX();

    static Transform2D ReflectionY();

    Transform2D& Combine(const Transform2D& rhs);

    [[nodiscard]] const glm::mat3& Matrix() const;

    [[nodiscard]] Mat3Uniform ToWgslMat3Uniform() const;

    static Mat3Uniform ToWgslMat3Uniform(const glm::mat3& matrix);

    void Reset();

private:
    glm::mat3 m_matrix = glm::mat3(1.0f);
};

static_assert(sizeof(Transform2D::Mat3Uniform) == 12 * sizeof(float), "WGSL mat3 uniform must be 48 bytes");

#endif // BJTU_WGPU_RENDERER_TRANSFORM2D_H
