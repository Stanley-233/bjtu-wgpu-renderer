#ifndef BJTU_WGPU_RENDERER_ORTHOGRAPHICCAMERA_H
#define BJTU_WGPU_RENDERER_ORTHOGRAPHICCAMERA_H

#include "Camera.h"

class OrthographicCamera final : public Camera {
public:
    void SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    glm::mat4 View() const override;

    glm::mat4 Projection(float aspect) const override;

private:
    float m_left      = -1.0f;
    float m_right     = 1.0f;
    float m_bottom    = -1.0f;
    float m_top       = 1.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane  = 100.0f;
};

#endif // BJTU_WGPU_RENDERER_ORTHOGRAPHICCAMERA_H
