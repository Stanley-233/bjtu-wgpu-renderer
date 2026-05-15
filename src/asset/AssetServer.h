#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <type_traits>
#include <unordered_map>

#include "AssetId.h"
#include "types/ImageAsset.h"
#include "types/MaterialAsset.h"
#include "types/MeshAsset.h"
#include "types/ModelAsset.h"

class AssetServer {
public:
    AssetId<ImageAsset> CreateImage(ImageAsset image);

    AssetId<MeshAsset> CreateMesh(MeshAsset mesh);

    AssetId<MaterialAsset> CreateMaterial(MaterialAsset material);

    AssetId<ModelAsset> LoadModel(const std::filesystem::path& path);

    template <typename T>
    [[nodiscard]] const T* Get(AssetId<T> id) const;

private:
    AssetId<MeshAsset> StoreMesh(MeshAsset mesh);

    AssetId<MaterialAsset> StoreMaterial(MaterialAsset material);

    AssetId<ImageAsset> StoreImage(ImageAsset image);

    AssetId<ModelAsset> StoreModel(const std::filesystem::path& sourcePath, ModelAsset model);

    std::unordered_map<std::filesystem::path, AssetId<ModelAsset> > m_modelPathToId;
    std::deque<ImageAsset>                                          m_images;
    std::deque<MeshAsset>                                           m_meshes;
    std::deque<MaterialAsset>                                       m_materials;
    std::deque<ModelAsset>                                          m_models;
};

template <typename>
inline constexpr bool kAssetServerAlwaysFalse = false;

template <typename T>
const T* AssetServer::Get(AssetId<T> id) const {
    if (!id.IsValid()) {
        return nullptr;
    }

    const std::size_t index = id.value - 1;
    if constexpr (std::is_same_v<T, MeshAsset>) {
        if (index >= m_meshes.size()) {
            return nullptr;
        }
        return &m_meshes[index];
    } else if constexpr (std::is_same_v<T, MaterialAsset>) {
        if (index >= m_materials.size()) {
            return nullptr;
        }
        return &m_materials[index];
    } else if constexpr (std::is_same_v<T, ImageAsset>) {
        if (index >= m_images.size()) {
            return nullptr;
        }
        return &m_images[index];
    } else if constexpr (std::is_same_v<T, ModelAsset>) {
        if (index >= m_models.size()) {
            return nullptr;
        }
        return &m_models[index];
    } else {
        static_assert(kAssetServerAlwaysFalse<T>, "Unsupported asset type.");
    }
    return nullptr;
}
