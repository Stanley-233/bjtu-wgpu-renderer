#include "AssetServer.h"

#include <string>
#include <utility>

#include "importers/GltfImporter.h"

AssetHandle<MeshAsset> AssetServer::CreateMesh(MeshAsset mesh) {
    return StoreMesh(std::move(mesh));
}

AssetHandle<MaterialAsset> AssetServer::CreateMaterial(MaterialAsset material) {
    return StoreMaterial(std::move(material));
}

AssetHandle<ModelAsset> AssetServer::LoadModel(const std::filesystem::path& path) {
    if (const auto it = m_modelPathToId.find(path); it != m_modelPathToId.end()) {
        return AssetHandle{
            .id = it->second,
            .asset = GetModel(it->second),
        };
    }

    ModelAsset model{};
    bool imported = false;
    const std::string extension = path.extension().string();
    if (extension == ".gltf" || extension == ".glb") {
        imported = GltfImporter{}.Import(path, model);
    }

    if (!imported) {
        return {};
    }
    return StoreModel(path, std::move(model));
}

const MeshAsset* AssetServer::GetMesh(const AssetId<MeshAsset> id) const {
    if (!id.IsValid()) {
        return nullptr;
    }

    const std::size_t index = static_cast<std::size_t>(id.value - 1);
    if (index >= m_meshes.size()) {
        return nullptr;
    }
    return &m_meshes[index];
}

const MaterialAsset* AssetServer::GetMaterial(const AssetId<MaterialAsset> id) const {
    if (!id.IsValid()) {
        return nullptr;
    }

    const std::size_t index = static_cast<std::size_t>(id.value - 1);
    if (index >= m_materials.size()) {
        return nullptr;
    }
    return &m_materials[index];
}

const ModelAsset* AssetServer::GetModel(const AssetId<ModelAsset> id) const {
    if (!id.IsValid()) {
        return nullptr;
    }

    const std::size_t index = static_cast<std::size_t>(id.value - 1);
    if (index >= m_models.size()) {
        return nullptr;
    }
    return &m_models[index];
}

AssetHandle<MeshAsset> AssetServer::StoreMesh(MeshAsset mesh) {
    m_meshes.push_back(std::move(mesh));
    const AssetId<MeshAsset> id{static_cast<uint32_t>(m_meshes.size())};
    return AssetHandle<MeshAsset>{
        .id = id,
        .asset = &m_meshes.back(),
    };
}

AssetHandle<MaterialAsset> AssetServer::StoreMaterial(MaterialAsset material) {
    m_materials.push_back(std::move(material));
    const AssetId<MaterialAsset> id{static_cast<uint32_t>(m_materials.size())};
    return AssetHandle<MaterialAsset>{
        .id = id,
        .asset = &m_materials.back(),
    };
}

AssetHandle<ModelAsset> AssetServer::StoreModel(const std::filesystem::path& sourcePath, ModelAsset model) {
    m_models.push_back(std::move(model));
    const AssetId<ModelAsset> id{static_cast<uint32_t>(m_models.size())};
    m_modelPathToId[sourcePath] = id;
    return AssetHandle<ModelAsset>{
        .id = id,
        .asset = &m_models.back(),
    };
}
