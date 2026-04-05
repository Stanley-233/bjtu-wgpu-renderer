#ifndef BJTU_WGPU_RENDERER_PERSPECTIVECAMERA_H
#define BJTU_WGPU_RENDERER_PERSPECTIVECAMERA_H

#include "Camera.h"

class PerspectiveCamera final : public Camera {
public:
    void SetPerspective(float fovYRadians, float nearPlane, float farPlane);

    glm::mat4 View() const override;

    glm::mat4 Projection(float aspect) const override;

private:
    float m_fovYRadians = 1.0471976f;
    float m_nearPlane   = 0.1f;
    float m_farPlane    = 100.0f;
};

#endif // BJTU_WGPU_RENDERER_PERSPECTIVECAMERA_H
