#ifndef BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H
#define BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H

#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "asset/types/MaterialAsset.h"
#include "RenderLightSet.h"

struct alignas(16) SceneUniformData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec4 cameraPosition{0.0f, 0.0f, 0.0f, 1.0f};
    glm::uvec4 lightCounts{0U, 0U, 0U, 0U};
    DirectionalLightData directionalLight{};
    std::array<PointLightData, RenderLightSet::kMaxPointLights> pointLights{};
    std::array<SpotLightData, RenderLightSet::kMaxSpotLights> spotLights{};
};

struct alignas(16) DirectionalShadowUniformData {
    glm::mat4 lightViewProjection{1.0f};
    glm::vec4 shadowParams{0.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) ObjectUniformData {
    glm::mat4 model{1.0f};
    glm::mat4 normalMatrix{1.0f};
};

struct alignas(16) ShadowObjectUniformData {
    glm::mat4 model{1.0f};
};

struct alignas(16) MaterialUniformData {
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::uvec4 surfaceOptions{
        static_cast<uint32_t>(EMaterialShadingModel::Unlit),
        1U,
        0U,
        0U,
    };
};

#endif // BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H
