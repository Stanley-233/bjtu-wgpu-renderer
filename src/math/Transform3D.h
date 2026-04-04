#ifndef BJTU_WGPU_RENDERER_TRANSFORM3D_H
#define BJTU_WGPU_RENDERER_TRANSFORM3D_H

#include <glm/mat4x4.hpp>

class Transform3D {
public:
    // 根据 WGSL 标准，mat4不需要padding
    static constexpr uint64_t kMat4UniformSize = sizeof(glm::mat4);

    static Transform3D Identity();

    static Transform3D Translation(float tx, float ty);

    static Transform3D Rotation(float radians);

    static Transform3D Scale(float sx, float sy);

    static Transform3D Shear(float shx, float shy);

    enum EReflectionType {
        ReflectX, ReflectY, ReflectZ
    };

    static Transform3D Reflection(EReflectionType type);

    Transform3D& Combine(const Transform3D& rhs);

    [[nodiscard]] const glm::mat4& Matrix() const;

    void Reset();

private:
    glm::mat4 m_matrix = glm::mat4(1.0f);
};


#endif //BJTU_WGPU_RENDERER_TRANSFORM3D_H