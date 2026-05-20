#ifndef BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H
#define BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H

#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
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
    // x=metallicFactor, y=roughnessFactor, z=normalScale, w=reserved
    glm::vec4 pbrParams{1.0f, 1.0f, 1.0f, 0.0f};
    // x=baseColorTexCoord, y=normalTexCoord, z=metallicRoughnessTexCoord, w=reserved
    glm::uvec4 textureCoordSets{0U, 0U, 0U, 0U};
    glm::uvec4 surfaceOptions{
        static_cast<uint32_t>(EMaterialShadingModel::Unlit),
        1U,
        0U,
        0U,
    };
};

struct alignas(16) SsaoUniformData {
    glm::mat4 projection{1.0f};
    glm::mat4 invProjection{1.0f};
    // x=viewportWidth, y=viewportHeight, z=sampleRadius, w=reserved
    glm::vec4 viewportSizeAndRadius{1.0f, 1.0f, 0.5f, 0.0f};
    // x=bias, y=intensity, z=sampleCount, w=reserved
    glm::vec4 aoParams{0.025f, 1.5f, 16.0f, 0.0f};
};

#endif // BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H
