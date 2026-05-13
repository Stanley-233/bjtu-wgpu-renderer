#ifndef BJTU_WGPU_RENDERER_RENDERCAMERA_H
#define BJTU_WGPU_RENDERER_RENDERCAMERA_H

#include <glm/mat4x4.hpp>

struct RenderCamera {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};

#endif // BJTU_WGPU_RENDERER_RENDERCAMERA_H
