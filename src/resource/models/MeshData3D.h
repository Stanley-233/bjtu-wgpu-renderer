#ifndef BJTU_WGPU_RENDERER_MESHDATA3D_H
#define BJTU_WGPU_RENDERER_MESHDATA3D_H

#include <vector>

#include <glm/vec3.hpp>

struct Vertex3D {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
};

struct MeshData3D {
    std::vector<Vertex3D> vertices;
    std::vector<uint16_t> indices;
};

#endif // BJTU_WGPU_RENDERER_MESHDATA3D_H
