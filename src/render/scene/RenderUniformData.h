#ifndef BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H
#define BJTU_WGPU_RENDERER_RENDERUNIFORMDATA_H

#include <array>
#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "asset/types/MaterialAsset.h"
#include "RenderLightSet.h"

enum class EPbrDebugView : uint32_t {
    Off = 0,
    GeometricNormal = 1,
    NormalMapWorld = 2,
    NormalDelta = 3,
};

enum class EToneMapExposureMode : uint32_t {
    ManualEv = 0,
    AutoExposure = 1,
};

enum class EDoFDebugMode : uint32_t {
    Off = 0,
    FocusPlaneTint = 1,
};

struct ToneMapSettings {
    EToneMapExposureMode exposureMode = EToneMapExposureMode::ManualEv;
    float                exposureEv  = 0.0f;
};

struct DofSettings {
    bool           enabled                 = false;
    float          focusDistance           = 3.1f;
    float          focusRange              = 5.0f;
    float          maxBlurRadiusPixels     = 10.0f;
    float          debugPlaneHalfThickness = 0.08f;
    EDoFDebugMode  debugMode               = EDoFDebugMode::Off;
};

struct SsrSettings {
    bool   enabled     = true;
    float  strength    = 1.60f;
    float  maxDistance = 8.0f;
    float  thickness   = 0.65f;
};

struct alignas(16) SceneUniformData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    // cameraPosition.xyz = world-space camera position, cameraPosition.w = reserved
    glm::vec4 cameraPosition{0.0f, 0.0f, 0.0f, 1.0f};
    // lightCounts.x = directional count, y = point count, z = spot count, w = reserved
    glm::uvec4 lightCounts{0U, 0U, 0U, 0U};
    DirectionalLightData directionalLight{};
    // PointLightData / SpotLightData field semantics must stay in sync with RenderLightSet.h and WGSL.
    std::array<PointLightData, RenderLightSet::kMaxPointLights> pointLights{};
    std::array<SpotLightData, RenderLightSet::kMaxSpotLights> spotLights{};
};

struct alignas(16) PbrDebugUniformData {
    glm::uvec4 options{0U, 0U, 0U, 0U};
};

struct alignas(16) DirectionalShadowUniformData {
    glm::mat4 lightViewProjection{1.0f};
    glm::vec4 shadowParams{0.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) SkyboxUniformData {
    glm::mat4 invViewRotation{1.0f};
    glm::mat4 invProjection{1.0f};
};

struct alignas(16) ToneMapUniformData {
    // x=exposureMode, y=manualExposureEv, zw=reserved
    glm::vec4 params{0.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) DofUniformData {
    glm::mat4 invProjection{1.0f};
    // x=viewportWidth, y=viewportHeight, z=maxBlurRadiusPixels, w=passMode(0=coc, 1=blur)
    glm::vec4 viewportAndBlur{1.0f, 1.0f, 10.0f, 0.0f};
    // x=focusDistance, y=focusRange, z=debugPlaneHalfThickness, w=debugMode
    glm::vec4 focusParams{3.1f, 5.0f, 0.08f, 0.0f};
    // x=blurDirX, y=blurDirY, zw=reserved
    glm::vec4 blurDirection{1.0f, 0.0f, 0.0f, 0.0f};
};

struct alignas(16) SsrUniformData {
    glm::mat4 projection{1.0f};
    glm::mat4 invProjection{1.0f};
    // x=viewportWidth, y=viewportHeight, zw=reserved
    glm::vec4 viewport{1.0f, 1.0f, 0.0f, 0.0f};
    // x=strength, y=maxDistance, z=thickness, w=stepCount
    glm::vec4 params{1.40f, 8.0f, 0.18f, 32.0f};
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
    // x=shadingModel, y=useVertexColor, z=hasNormalTexture, w=doubleSided
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
