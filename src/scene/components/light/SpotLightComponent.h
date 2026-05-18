#ifndef BJTU_WGPU_RENDERER_SPOTLIGHTCOMPONENT_H
#define BJTU_WGPU_RENDERER_SPOTLIGHTCOMPONENT_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct SpotLightComponent {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float     intensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float     range = 10.0f;
    float     innerConeAngle = 0.25f;
    float     outerConeAngle = 0.5f;
};

#endif // BJTU_WGPU_RENDERER_SPOTLIGHTCOMPONENT_H
