#ifndef BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
#define BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H

#include <optional>

#include "asset/AssetId.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"

struct StaticMeshComponent {
    AssetId<MeshAsset>     mesh{};
    AssetId<MaterialAsset> material{};
    std::optional<EMaterialShadingModel> shadingModelOverride{};

    void SetShadingModelOverride(const EMaterialShadingModel shadingModel) {
        shadingModelOverride = shadingModel;
    }

    void ClearShadingModelOverride() {
        shadingModelOverride.reset();
    }

    [[nodiscard]] EMaterialShadingModel ResolveShadingModel(const MaterialAsset* materialAsset) const {
        if (shadingModelOverride.has_value()) {
            return *shadingModelOverride;
        }
        if (materialAsset != nullptr) {
            return materialAsset->shadingModel;
        }
        return EMaterialShadingModel::Unlit;
    }
};

#endif // BJTU_WGPU_RENDERER_STATICMESHCOMPONENT_H
