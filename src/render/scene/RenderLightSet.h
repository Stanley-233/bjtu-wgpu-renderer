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
    // direction.xyz = normalized world direction, direction.w = reserved
    glm::vec4 direction{0.0f, -1.0f, 0.0f, 0.0f};
    // color.rgb = light color * intensity, color.w = reserved
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct alignas(16) PointLightData {
    // position.xyz = world position, position.w = reserved (1.0)
    glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    // color.rgb = light color * intensity, color.w = reserved
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    // attenuation.xyz = legacy constant / linear / quadratic, attenuation.w = range
    glm::vec4 attenuation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) SpotLightData {
    // position.xyz = world position, position.w = reserved (1.0)
    glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    // direction.xyz = normalized world direction, direction.w = reserved
    glm::vec4 direction{0.0f, -1.0f, 0.0f, 0.0f};
    // color.rgb = light color * intensity, color.w = reserved
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    // angles.xy = cos(inner) / cos(outer), angles.z = range, angles.w = reserved
    glm::vec4 angles{0.0f, 0.0f, 0.0f, 0.0f};
};

static_assert(alignof(PointLightData) == 16, "PointLightData must remain 16-byte aligned.");
static_assert(sizeof(PointLightData) == sizeof(glm::vec4) * 3, "PointLightData layout changed.");
static_assert(alignof(SpotLightData) == 16, "SpotLightData must remain 16-byte aligned.");
static_assert(sizeof(SpotLightData) == sizeof(glm::vec4) * 4, "SpotLightData layout changed.");

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
