#pragma once

#include <cstdint>
#include <optional>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "asset/AssetId.h"
#include "ImageAsset.h"

enum class EMaterialShadingModel : uint32_t {
    Unlit = 0,
    Lambert = 1,
    Pbr = 2,
};

struct MaterialTextureAsset {
    AssetId<ImageAsset> image{};
    uint32_t            texCoord = 0;

    [[nodiscard]] bool IsValid() const {
        return image.IsValid();
    }
};

struct SpecularExtensionAsset {
    float                                   specularFactor = 1.0f;
    glm::vec3                               specularColorFactor{1.0f, 1.0f, 1.0f};
    std::optional<MaterialTextureAsset>     specularTexture{};
    std::optional<MaterialTextureAsset>     specularColorTexture{};
};

struct MaterialAsset {
    EMaterialShadingModel                 shadingModel{EMaterialShadingModel::Unlit};
    glm::vec4                             baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    std::optional<MaterialTextureAsset>   baseColorTexture{};
    float                                 metallicFactor  = 1.0f;
    float                                 roughnessFactor = 1.0f;
    std::optional<MaterialTextureAsset>   metallicRoughnessTexture{};
    std::optional<MaterialTextureAsset>   normalTexture{};
    float                                 normalScale = 1.0f;
    std::optional<SpecularExtensionAsset> specular{};
    bool                                  useVertexColor = true;
};
