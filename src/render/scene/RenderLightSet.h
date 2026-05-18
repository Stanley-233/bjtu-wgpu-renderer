#ifndef BJTU_WGPU_RENDERER_RENDERLIGHTSET_H
#define BJTU_WGPU_RENDERER_RENDERLIGHTSET_H

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/vec4.hpp>

enum class ELightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2,
};

struct alignas(16) DirectionalLightData {
    glm::vec4 direction{0.0f, -1.0f, 0.0f, 0.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct alignas(16) PointLightData {
    glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 attenuation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) SpotLightData {
    glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 direction{0.0f, -1.0f, 0.0f, 0.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 angles{0.0f, 0.0f, 0.0f, 0.0f};
};

struct RenderLightSet {
    static constexpr std::size_t kMaxPointLights = 8;
    static constexpr std::size_t kMaxSpotLights = 8;

    DirectionalLightData directionalLight{};
    std::array<PointLightData, kMaxPointLights> pointLights{};
    std::array<SpotLightData, kMaxSpotLights> spotLights{};
    uint32_t directionalLightCount = 0;
    uint32_t pointLightCount = 0;
    uint32_t spotLightCount = 0;
};

#endif // BJTU_WGPU_RENDERER_RENDERLIGHTSET_H
