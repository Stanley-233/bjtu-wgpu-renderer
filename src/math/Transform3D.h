#ifndef BJTU_WGPU_RENDERER_TRANSFORM3D_H
#define BJTU_WGPU_RENDERER_TRANSFORM3D_H

#include <glm/mat4x4.hpp>

class Transform3D {
public:
    static constexpr uint64_t kMat4UniformSize = sizeof(glm::mat4);

    static Transform3D Identity();

    static Transform3D Translation(float tx, float ty, float tz);

    static Transform3D RotationX(float radians);

    static Transform3D RotationY(float radians);

    static Transform3D RotationZ(float radians);

    static Transform3D Scale(float sx, float sy, float sz);

    Transform3D& Combine(const Transform3D& rhs);

    [[nodiscard]] const glm::mat4& Matrix() const;

    void Reset();

private:
    glm::mat4 m_matrix = glm::mat4(1.0f);
};


#endif //BJTU_WGPU_RENDERER_TRANSFORM3D_H
