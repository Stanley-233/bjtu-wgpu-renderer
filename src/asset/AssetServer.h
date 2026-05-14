#pragma once

#include <deque>
#include <filesystem>
#include <unordered_map>

#include "AssetHandle.h"
#include "types/MaterialAsset.h"
#include "types/MeshAsset.h"
#include "types/ModelAsset.h"

class AssetServer {
public:
    AssetHandle<MeshAsset> CreateMesh(MeshAsset mesh);

    AssetHandle<MaterialAsset> CreateMaterial(MaterialAsset material);

    AssetHandle<ModelAsset> LoadModel(const std::filesystem::path& path);

    [[nodiscard]] const MeshAsset* GetMesh(AssetId<MeshAsset> id) const;

    [[nodiscard]] const MaterialAsset* GetMaterial(AssetId<MaterialAsset> id) const;

    [[nodiscard]] const ModelAsset* GetModel(AssetId<ModelAsset> id) const;

private:
    AssetHandle<MeshAsset> StoreMesh(MeshAsset mesh);

    AssetHandle<MaterialAsset> StoreMaterial(MaterialAsset material);

    AssetHandle<ModelAsset> StoreModel(const std::filesystem::path& sourcePath, ModelAsset model);

    std::unordered_map<std::filesystem::path, AssetId<ModelAsset> > m_modelPathToId;
    std::deque<MeshAsset>                                           m_meshes;
    std::deque<MaterialAsset>                                       m_materials;
    std::deque<ModelAsset>                                          m_models;
};
