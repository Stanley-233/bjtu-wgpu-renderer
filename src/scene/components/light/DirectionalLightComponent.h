#ifndef BJTU_WGPU_RENDERER_DIRECTIONALLIGHTCOMPONENT_H
#define BJTU_WGPU_RENDERER_DIRECTIONALLIGHTCOMPONENT_H

#include <glm/vec3.hpp>

struct DirectionalLightComponent {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float     intensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
};

#endif // BJTU_WGPU_RENDERER_DIRECTIONALLIGHTCOMPONENT_H
