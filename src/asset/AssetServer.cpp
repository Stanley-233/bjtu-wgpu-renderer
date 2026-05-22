#include "AssetServer.h"

#include <string>
#include <utility>

#include "importers/GltfImporter.h"
#include "importers/HdrImageImporter.h"

AssetId<ImageAsset> AssetServer::CreateImage(ImageAsset image) {
    return StoreImage(std::move(image));
}

AssetId<HdrImageAsset> AssetServer::LoadHdrImage(const std::filesystem::path& path) {
    if (const auto it = m_hdrImagePathToId.find(path); it != m_hdrImagePathToId.end()) {
        return it->second;
    }

    HdrImageAsset image{};
    if (!HdrImageImporter::Import(path, image)) {
        return {};
    }
    return StoreHdrImage(path, std::move(image));
}

AssetId<MeshAsset> AssetServer::CreateMesh(MeshAsset mesh) {
    return StoreMesh(std::move(mesh));
}

AssetId<MaterialAsset> AssetServer::CreateMaterial(MaterialAsset material) {
    return StoreMaterial(std::move(material));
}

AssetId<ModelAsset> AssetServer::LoadModel(const std::filesystem::path& path) {
    if (const auto it = m_modelPathToId.find(path); it != m_modelPathToId.end()) {
        return it->second;
    }

    ModelAsset model{};
    bool imported = false;
    const std::string extension = path.extension().string();
    if (extension == ".gltf" || extension == ".glb") {
        imported = GltfImporter{}.Import(path, *this, model);
    }

    if (!imported) {
        return {};
    }
    return StoreModel(path, std::move(model));
}

AssetId<MeshAsset> AssetServer::StoreMesh(MeshAsset mesh) {
    m_meshes.push_back(std::move(mesh));
    const AssetId<MeshAsset> id{static_cast<uint32_t>(m_meshes.size())};
    return id;
}

AssetId<MaterialAsset> AssetServer::StoreMaterial(MaterialAsset material) {
    m_materials.push_back(std::move(material));
    const AssetId<MaterialAsset> id{static_cast<uint32_t>(m_materials.size())};
    return id;
}

AssetId<ImageAsset> AssetServer::StoreImage(ImageAsset image) {
    m_images.push_back(std::move(image));
    const AssetId<ImageAsset> id{static_cast<uint32_t>(m_images.size())};
    return id;
}

AssetId<HdrImageAsset> AssetServer::StoreHdrImage(const std::filesystem::path& sourcePath, HdrImageAsset image) {
    m_hdrImages.push_back(std::move(image));
    const AssetId<HdrImageAsset> id{static_cast<uint32_t>(m_hdrImages.size())};
    m_hdrImagePathToId[sourcePath] = id;
    return id;
}

AssetId<ModelAsset> AssetServer::StoreModel(const std::filesystem::path& sourcePath, ModelAsset model) {
    m_models.push_back(std::move(model));
    const AssetId<ModelAsset> id{static_cast<uint32_t>(m_models.size())};
    m_modelPathToId[sourcePath] = id;
    return id;
}
