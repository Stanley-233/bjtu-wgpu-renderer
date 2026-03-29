#ifndef BJTU_WGPU_RENDERER_TRANSFORM2D_H
#define BJTU_WGPU_RENDERER_TRANSFORM2D_H

#include <glm/mat3x3.hpp>

class Transform2D {
public:
    static Transform2D Identity();

    static Transform2D Translation(float tx, float ty);

    static Transform2D Rotation(float radians);

    static Transform2D Scale(float sx, float sy);

    static Transform2D Shear(float shx, float shy);

    static Transform2D ReflectionX();

    static Transform2D ReflectionY();

    Transform2D& Combine(const Transform2D& rhs);

    [[nodiscard]] const glm::mat3& Matrix() const;

    void Reset();

private:
    glm::mat3 m_matrix = glm::mat3(1.0f);
};

#endif // BJTU_WGPU_RENDERER_TRANSFORM2D_H
