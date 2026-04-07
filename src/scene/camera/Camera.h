#ifndef BJTU_WGPU_RENDERER_CAMERA_H
#define BJTU_WGPU_RENDERER_CAMERA_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    virtual ~Camera() = default;

    virtual glm::mat4 View() const = 0;

    virtual glm::mat4 Projection(float aspect) const = 0;

    void SetPose(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);

    [[nodiscard]] const glm::vec3& Position() const;

    [[nodiscard]] const glm::vec3& Target() const;

    [[nodiscard]] const glm::vec3& Up() const;

protected:
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_target{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
};

#endif // BJTU_WGPU_RENDERER_CAMERA_H
