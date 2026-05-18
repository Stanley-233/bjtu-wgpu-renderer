#ifndef BJTU_WGPU_RENDERER_POINTLIGHTCOMPONENT_H
#define BJTU_WGPU_RENDERER_POINTLIGHTCOMPONENT_H

#include <glm/vec3.hpp>

struct PointLightComponent {
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float     intensity = 1.0f;
    float     range = 10.0f;
};

#endif // BJTU_WGPU_RENDERER_POINTLIGHTCOMPONENT_H
