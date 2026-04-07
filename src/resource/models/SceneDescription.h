#ifndef BJTU_WGPU_RENDERER_SCENEDESCRIPTION_H
#define BJTU_WGPU_RENDERER_SCENEDESCRIPTION_H

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "MeshData3D.h"

enum class ECameraProjectionType {
    Perspective,
    Orthographic,
};

struct CameraDescription {
    ECameraProjectionType projectionType = ECameraProjectionType::Perspective;
    glm::vec3             position{0.0f, 0.0f, 0.0f};
    glm::vec3             target{0.0f, 0.0f, -1.0f};
    glm::vec3             up{0.0f, 1.0f, 0.0f};
};

struct ObjectDescription {
    std::string name{};
    glm::vec3  translation{0.0f, 0.0f, 0.0f};
    glm::vec3  rotation{0.0f, 0.0f, 0.0f};
    glm::vec3  scale{1.0f, 1.0f, 1.0f};
    MeshData3D mesh;
};

struct SceneDescription {
    CameraDescription               camera;
    std::vector<ObjectDescription> objects;
};

#endif // BJTU_WGPU_RENDERER_SCENEDESCRIPTION_H
