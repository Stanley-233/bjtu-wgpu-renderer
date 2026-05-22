#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION

#include "GltfImporter.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>

#include "asset/AssetServer.h"
#include "asset/types/ImageAsset.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"

static glm::mat4 ComposeNodeMatrix(const tinygltf::Node& node) {
    if (node.matrix.size() == 16U) {
        glm::mat4 matrix{1.0f};
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                matrix[column][row] = static_cast<float>(node.matrix[static_cast<std::size_t>(column * 4 + row)]);
            }
        }
        return matrix;
    }

    glm::mat4 matrix{1.0f};
    if (node.translation.size() == 3U) {
        matrix = glm::translate(matrix, glm::vec3{
                                            static_cast<float>(node.translation[0]),
                                            static_cast<float>(node.translation[1]),
                                            static_cast<float>(node.translation[2]),
                                        });
    }
    if (node.rotation.size() == 4U) {
        const glm::quat rotation{
            static_cast<float>(node.rotation[3]),
            static_cast<float>(node.rotation[0]),
            static_cast<float>(node.rotation[1]),
            static_cast<float>(node.rotation[2]),
        };
        matrix *= glm::mat4_cast(rotation);
    }
    if (node.scale.size() == 3U) {
        matrix = glm::scale(matrix, glm::vec3{
                                          static_cast<float>(node.scale[0]),
                                          static_cast<float>(node.scale[1]),
                                          static_cast<float>(node.scale[2]),
                                      });
    }
    return matrix;
}

static bool ReadAccessorVec3(const tinygltf::Model& model, const int accessorIndex, std::vector<glm::vec3>& outValues) {
    if (accessorIndex < 0) {
        return false;
    }

    const tinygltf::Accessor& accessor = model.accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor.bufferView < 0 || accessor.type != TINYGLTF_TYPE_VEC3 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return false;
    }

    const tinygltf::BufferView& bufferView = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer = model.buffers[static_cast<std::size_t>(bufferView.buffer)];
    const int stride = accessor.ByteStride(bufferView);
    if (stride <= 0) {
        return false;
    }

    outValues.resize(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t byteOffset = static_cast<std::size_t>(bufferView.byteOffset + accessor.byteOffset) + i * static_cast<std::size_t>(stride);
        if (byteOffset + sizeof(float) * 3U > buffer.data.size()) {
            return false;
        }
        const auto* values = reinterpret_cast<const float*>(buffer.data.data() + byteOffset);
        outValues[i] = glm::vec3{values[0], values[1], values[2]};
    }
    return true;
}

static bool ReadAccessorVec4(const tinygltf::Model& model, const int accessorIndex, std::vector<glm::vec4>& outValues) {
    if (accessorIndex < 0) {
        return false;
    }

    const tinygltf::Accessor& accessor = model.accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor.bufferView < 0 || accessor.type != TINYGLTF_TYPE_VEC4 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return false;
    }

    const tinygltf::BufferView& bufferView = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer = model.buffers[static_cast<std::size_t>(bufferView.buffer)];
    const int stride = accessor.ByteStride(bufferView);
    if (stride <= 0) {
        return false;
    }

    outValues.resize(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t byteOffset = static_cast<std::size_t>(bufferView.byteOffset + accessor.byteOffset) + i * static_cast<std::size_t>(stride);
        if (byteOffset + sizeof(float) * 4U > buffer.data.size()) {
            return false;
        }
        const auto* values = reinterpret_cast<const float*>(buffer.data.data() + byteOffset);
        outValues[i] = glm::vec4{values[0], values[1], values[2], values[3]};
    }
    return true;
}

static bool ReadAccessorVec2(const tinygltf::Model& model, const int accessorIndex, std::vector<glm::vec2>& outValues) {
    if (accessorIndex < 0) {
        return false;
    }

    const tinygltf::Accessor& accessor = model.accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor.bufferView < 0 || accessor.type != TINYGLTF_TYPE_VEC2 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return false;
    }

    const tinygltf::BufferView& bufferView = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer = model.buffers[static_cast<std::size_t>(bufferView.buffer)];
    const int stride = accessor.ByteStride(bufferView);
    if (stride <= 0) {
        return false;
    }

    outValues.resize(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t byteOffset = static_cast<std::size_t>(bufferView.byteOffset + accessor.byteOffset) + i * static_cast<std::size_t>(stride);
        if (byteOffset + sizeof(float) * 2U > buffer.data.size()) {
            return false;
        }
        const auto* values = reinterpret_cast<const float*>(buffer.data.data() + byteOffset);
        outValues[i] = glm::vec2{values[0], values[1]};
    }
    return true;
}

static float NormalizeColorComponent(const int componentType, const std::uint8_t* data) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return *reinterpret_cast<const float*>(data);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return static_cast<float>(*data) / 255.0f;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return static_cast<float>(*reinterpret_cast<const uint16_t*>(data)) / 65535.0f;
    default:
        return 1.0f;
    }
}

static uint32_t ComponentByteSize(const int componentType) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return sizeof(float);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return sizeof(std::uint8_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return sizeof(std::uint16_t);
    default:
        return 0;
    }
}

static bool ReadAccessorColor(const tinygltf::Model& model, const int accessorIndex, std::vector<glm::vec4>& outValues) {
    if (accessorIndex < 0) {
        return false;
    }

    const tinygltf::Accessor& accessor = model.accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor.bufferView < 0 || (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)) {
        return false;
    }
    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT
        && accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE
        && accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        return false;
    }

    const tinygltf::BufferView& bufferView = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer = model.buffers[static_cast<std::size_t>(bufferView.buffer)];
    const int stride = accessor.ByteStride(bufferView);
    const uint32_t componentSize = ComponentByteSize(accessor.componentType);
    const uint32_t componentCount = accessor.type == TINYGLTF_TYPE_VEC3 ? 3U : 4U;
    if (stride <= 0 || componentSize == 0U) {
        return false;
    }

    outValues.resize(accessor.count, glm::vec4{1.0f});
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t byteOffset = static_cast<std::size_t>(bufferView.byteOffset + accessor.byteOffset)
                                       + i * static_cast<std::size_t>(stride);
        const std::size_t requiredBytes = static_cast<std::size_t>(componentSize) * componentCount;
        if (byteOffset + requiredBytes > buffer.data.size()) {
            return false;
        }

        const std::uint8_t* componentData = buffer.data.data() + byteOffset;
        glm::vec4 color{1.0f};
        color.r = NormalizeColorComponent(accessor.componentType, componentData + componentSize * 0U);
        color.g = NormalizeColorComponent(accessor.componentType, componentData + componentSize * 1U);
        color.b = NormalizeColorComponent(accessor.componentType, componentData + componentSize * 2U);
        if (componentCount == 4U) {
            color.a = NormalizeColorComponent(accessor.componentType, componentData + componentSize * 3U);
        }
        outValues[i] = glm::clamp(color, 0.0f, 1.0f);
    }

    return true;
}

static bool ReadIndices(const tinygltf::Model& model, const int accessorIndex, std::vector<uint16_t>& outIndices) {
    const auto fail = [&](const std::string& message) -> bool {
        std::cerr << "[GltfImporter] Warning: failed to read indices for accessor " << accessorIndex
                  << ": " << message << std::endl;
        return false;
    };

    const tinygltf::Accessor& accessor = model.accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor.bufferView < 0 || accessor.type != TINYGLTF_TYPE_SCALAR) {
        return fail("accessor is missing a buffer view or is not a scalar index buffer");
    }

    const tinygltf::BufferView& bufferView = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer = model.buffers[static_cast<std::size_t>(bufferView.buffer)];
    const int stride = accessor.ByteStride(bufferView);
    if (stride <= 0) {
        return fail("accessor stride is invalid");
    }

    outIndices.resize(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t byteOffset = static_cast<std::size_t>(bufferView.byteOffset + accessor.byteOffset) + i * static_cast<std::size_t>(stride);
        if (byteOffset >= buffer.data.size()) {
            return fail("computed byte offset exceeded buffer size");
        }

        switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            outIndices[i] = static_cast<uint16_t>(buffer.data[byteOffset]);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            if (byteOffset + sizeof(uint16_t) > buffer.data.size()) {
                return fail("uint16 index read exceeded buffer size");
            }
            const auto* values = reinterpret_cast<const uint16_t*>(buffer.data.data() + byteOffset);
            outIndices[i] = *values;
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            if (byteOffset + sizeof(uint32_t) > buffer.data.size()) {
                return fail("uint32 index read exceeded buffer size");
            }
            const auto* values = reinterpret_cast<const uint32_t*>(buffer.data.data() + byteOffset);
            if (*values > std::numeric_limits<uint16_t>::max()) {
                return fail("uint32 index value exceeds uint16 range; the renderer currently only supports uint16 index buffers");
            }
            outIndices[i] = static_cast<uint16_t>(*values);
            break;
        }
        default:
            return fail("unsupported index component type " + std::to_string(accessor.componentType));
        }
    }
    return true;
}

static ImageAsset DecodeImage(const tinygltf::Image& image, const EImageAssetFormat format) {
    ImageAsset asset{};
    asset.width = static_cast<uint32_t>(std::max(0, image.width));
    asset.height = static_cast<uint32_t>(std::max(0, image.height));
    asset.format = format;

    if (asset.width == 0U || asset.height == 0U || image.image.empty()) {
        return asset;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(asset.width) * static_cast<std::size_t>(asset.height);
    asset.pixels.resize(pixelCount * 4U, 255U);

    const int componentCount = std::max(1, image.component);
    for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
        const std::size_t sourceOffset = pixelIndex * static_cast<std::size_t>(componentCount);
        const std::size_t targetOffset = pixelIndex * 4U;
        if (sourceOffset >= image.image.size()) {
            break;
        }

        const auto readComponent = [&](const std::size_t componentIndex) -> std::uint8_t {
            if (sourceOffset + componentIndex >= image.image.size()) {
                return 255U;
            }
            return image.image[sourceOffset + componentIndex];
        };

        switch (componentCount) {
        case 1:
            asset.pixels[targetOffset + 0U] = readComponent(0U);
            asset.pixels[targetOffset + 1U] = readComponent(0U);
            asset.pixels[targetOffset + 2U] = readComponent(0U);
            asset.pixels[targetOffset + 3U] = 255U;
            break;
        case 2:
            asset.pixels[targetOffset + 0U] = readComponent(0U);
            asset.pixels[targetOffset + 1U] = readComponent(0U);
            asset.pixels[targetOffset + 2U] = readComponent(0U);
            asset.pixels[targetOffset + 3U] = readComponent(1U);
            break;
        case 3:
            asset.pixels[targetOffset + 0U] = readComponent(0U);
            asset.pixels[targetOffset + 1U] = readComponent(1U);
            asset.pixels[targetOffset + 2U] = readComponent(2U);
            asset.pixels[targetOffset + 3U] = 255U;
            break;
        default:
            asset.pixels[targetOffset + 0U] = readComponent(0U);
            asset.pixels[targetOffset + 1U] = readComponent(1U);
            asset.pixels[targetOffset + 2U] = readComponent(2U);
            asset.pixels[targetOffset + 3U] = readComponent(3U);
            break;
        }
    }

    return asset;
}

static AssetId<ImageAsset> LoadTextureImage(
    const tinygltf::Model& model,
    const int textureIndex,
    AssetServer& assetServer,
    const EImageAssetFormat format) {
    if (textureIndex < 0 || static_cast<std::size_t>(textureIndex) >= model.textures.size()) {
        return {};
    }
    const tinygltf::Texture& texture = model.textures[static_cast<std::size_t>(textureIndex)];
    if (texture.source < 0 || static_cast<std::size_t>(texture.source) >= model.images.size()) {
        return {};
    }
    return assetServer.CreateImage(DecodeImage(model.images[static_cast<std::size_t>(texture.source)], format));
}

static std::optional<MaterialTextureAsset> LoadTextureAsset(
    const tinygltf::Model& model,
    const int textureIndex,
    const int texCoord,
    AssetServer& assetServer,
    const EImageAssetFormat format) {
    const AssetId<ImageAsset> imageId = LoadTextureImage(model, textureIndex, assetServer, format);
    if (!imageId.IsValid()) {
        return std::nullopt;
    }

    MaterialTextureAsset textureAsset{};
    textureAsset.image = imageId;
    textureAsset.texCoord = texCoord >= 0 ? static_cast<uint32_t>(texCoord) : 0U;
    return textureAsset;
}

static bool ReadExtensionFloat(const tinygltf::Value& objectValue, const char* key, float& outValue) {
    if (!objectValue.IsObject() || !objectValue.Has(key)) {
        return false;
    }
    const tinygltf::Value& value = objectValue.Get(key);
    if (!value.IsNumber()) {
        return false;
    }
    outValue = static_cast<float>(value.GetNumberAsDouble());
    return true;
}

static bool ReadExtensionVec3(const tinygltf::Value& objectValue, const char* key, glm::vec3& outValue) {
    if (!objectValue.IsObject() || !objectValue.Has(key)) {
        return false;
    }
    const tinygltf::Value& value = objectValue.Get(key);
    if (!value.IsArray() || value.ArrayLen() != 3U) {
        return false;
    }
    glm::vec3 parsedValue{1.0f, 1.0f, 1.0f};
    for (std::size_t i = 0; i < 3U; ++i) {
        const tinygltf::Value& component = value.Get(i);
        if (!component.IsNumber()) {
            return false;
        }
        parsedValue[static_cast<glm::vec3::length_type>(i)] = static_cast<float>(component.GetNumberAsDouble());
    }
    outValue = parsedValue;
    return true;
}

static std::optional<MaterialTextureAsset> ReadExtensionTextureAsset(
    const tinygltf::Model& model,
    const tinygltf::Value& objectValue,
    const char* key,
    AssetServer& assetServer,
    const EImageAssetFormat format) {
    if (!objectValue.IsObject() || !objectValue.Has(key)) {
        return std::nullopt;
    }

    const tinygltf::Value& textureValue = objectValue.Get(key);
    if (!textureValue.IsObject() || !textureValue.Has("index")) {
        return std::nullopt;
    }

    const tinygltf::Value& indexValue = textureValue.Get("index");
    if (!indexValue.IsNumber()) {
        return std::nullopt;
    }

    int texCoord = 0;
    if (textureValue.Has("texCoord")) {
        const tinygltf::Value& texCoordValue = textureValue.Get("texCoord");
        if (texCoordValue.IsNumber()) {
            texCoord = texCoordValue.GetNumberAsInt();
        }
    }

    return LoadTextureAsset(
        model,
        indexValue.GetNumberAsInt(),
        texCoord,
        assetServer,
        format);
}

static std::optional<SpecularExtensionAsset> ReadSpecularExtension(
    const tinygltf::Model& model,
    const tinygltf::Material& material,
    AssetServer& assetServer) {
    const auto extensionIt = material.extensions.find("KHR_materials_specular");
    if (extensionIt == material.extensions.end()) {
        return std::nullopt;
    }
    if (!extensionIt->second.IsObject()) {
        return std::nullopt;
    }

    SpecularExtensionAsset specular{};
    (void)ReadExtensionFloat(extensionIt->second, "specularFactor", specular.specularFactor);
    (void)ReadExtensionVec3(extensionIt->second, "specularColorFactor", specular.specularColorFactor);
    specular.specularTexture = ReadExtensionTextureAsset(
        model,
        extensionIt->second,
        "specularTexture",
        assetServer,
        EImageAssetFormat::Rgba8Unorm);
    specular.specularColorTexture = ReadExtensionTextureAsset(
        model,
        extensionIt->second,
        "specularColorTexture",
        assetServer,
        EImageAssetFormat::Rgba8Unorm);
    return specular;
}

static bool IsSupportedExtension(const std::string_view extensionName) {
    return extensionName == "KHR_materials_unlit"
           || extensionName == "KHR_materials_specular";
}

static bool IsMaskedAlphaMode(const std::string_view alphaMode) {
    return alphaMode == "MASK" || alphaMode == "MASKED";
}

static void WarnMaskedAlphaMode(const tinygltf::Model& model, const std::filesystem::path& path) {
    for (const tinygltf::Material& material : model.materials) {
        if (!IsMaskedAlphaMode(material.alphaMode)) {
            continue;
        }

        std::cerr << "[GltfImporter] Warning: alphaMode '"
                  << material.alphaMode
                  << "' is not supported in '"
                  << path.string()
                  << "'; ignoring alpha mask and loading the material without MASK support."
                  << std::endl;
        return;
    }
}

static void WarnUnsupportedExtension(
    const std::string_view extensionName,
    const std::filesystem::path& path,
    std::unordered_set<std::string>& warnedExtensions) {
    if (IsSupportedExtension(extensionName) || warnedExtensions.contains(std::string{extensionName})) {
        return;
    }

    std::cerr << "[GltfImporter] Warning: unsupported extension '" << extensionName
              << "' in '" << path.string() << "'." << std::endl;
    warnedExtensions.insert(std::string{extensionName});
}

static void WarnUnsupportedExtensions(const tinygltf::Model& model, const std::filesystem::path& path) {
    std::unordered_set<std::string> warnedExtensions{};
    for (const std::string& extensionName : model.extensionsUsed) {
        WarnUnsupportedExtension(extensionName, path, warnedExtensions);
    }
    for (const std::string& extensionName : model.extensionsRequired) {
        WarnUnsupportedExtension(extensionName, path, warnedExtensions);
    }
    for (const tinygltf::Material& material : model.materials) {
        for (const auto& [extensionName, _] : material.extensions) {
            (void)_;
            WarnUnsupportedExtension(extensionName, path, warnedExtensions);
        }
    }
}

static MeshAsset BuildMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
    MeshAsset mesh{};

    const auto positionIt = primitive.attributes.find("POSITION");
    if (positionIt == primitive.attributes.end()) {
        return mesh;
    }

    const tinygltf::Accessor& positionAccessor = model.accessors[static_cast<std::size_t>(positionIt->second)];
    if (positionAccessor.bufferView < 0 || positionAccessor.type != TINYGLTF_TYPE_VEC3 || positionAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return mesh;
    }

    const tinygltf::BufferView& positionBufferView = model.bufferViews[static_cast<std::size_t>(positionAccessor.bufferView)];
    const tinygltf::Buffer& positionBuffer = model.buffers[static_cast<std::size_t>(positionBufferView.buffer)];
    const int positionStride = positionAccessor.ByteStride(positionBufferView);
    if (positionStride <= 0) {
        return mesh;
    }

    std::vector<glm::vec3> normals{};
    std::vector<glm::vec4> tangents{};
    std::vector<glm::vec2> texcoords0{};
    std::vector<glm::vec2> texcoords1{};
    std::vector<glm::vec4> colors{};
    const auto normalIt = primitive.attributes.find("NORMAL");
    if (normalIt != primitive.attributes.end()) {
        (void)ReadAccessorVec3(model, normalIt->second, normals);
    }
    const auto tangentIt = primitive.attributes.find("TANGENT");
    if (tangentIt != primitive.attributes.end()) {
        (void)ReadAccessorVec4(model, tangentIt->second, tangents);
    }
    const auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
    if (texcoordIt != primitive.attributes.end()) {
        (void)ReadAccessorVec2(model, texcoordIt->second, texcoords0);
    }
    const auto texcoord1It = primitive.attributes.find("TEXCOORD_1");
    if (texcoord1It != primitive.attributes.end()) {
        (void)ReadAccessorVec2(model, texcoord1It->second, texcoords1);
    }
    const auto colorIt = primitive.attributes.find("COLOR_0");
    if (colorIt != primitive.attributes.end()) {
        (void)ReadAccessorColor(model, colorIt->second, colors);
    }

    if (primitive.indices >= 0) {
        if (!ReadIndices(model, primitive.indices, mesh.indices)) {
            return {};
        }
    } else {
        mesh.indices.reserve(positionAccessor.count);
        for (std::size_t i = 0; i < positionAccessor.count; ++i) {
            if (i > std::numeric_limits<uint16_t>::max()) {
                return {};
            }
            mesh.indices.push_back(static_cast<uint16_t>(i));
        }
    }

    mesh.vertices.resize(positionAccessor.count);
    for (std::size_t vertexIndex = 0; vertexIndex < positionAccessor.count; ++vertexIndex) {
        const std::size_t byteOffset = static_cast<std::size_t>(positionBufferView.byteOffset + positionAccessor.byteOffset) + vertexIndex * static_cast<std::size_t>(positionStride);
        if (byteOffset + sizeof(float) * 3U > positionBuffer.data.size()) {
            return {};
        }
        const auto* values = reinterpret_cast<const float*>(positionBuffer.data.data() + byteOffset);
        mesh.vertices[vertexIndex].position = glm::vec3{values[0], values[1], values[2]};
        if (vertexIndex < normals.size()) {
            mesh.vertices[vertexIndex].normal = normals[vertexIndex];
        }
        if (vertexIndex < tangents.size()) {
            mesh.vertices[vertexIndex].tangent = tangents[vertexIndex];
        }
        if (vertexIndex < texcoords0.size()) {
            mesh.vertices[vertexIndex].uv0 = texcoords0[vertexIndex];
        }
        if (vertexIndex < texcoords1.size()) {
            mesh.vertices[vertexIndex].uv1 = texcoords1[vertexIndex];
        }
        if (vertexIndex < colors.size()) {
            mesh.vertices[vertexIndex].color = colors[vertexIndex];
        }
    }

    mesh.primitiveRanges.push_back(MeshPrimitiveRange{
        .firstIndex = 0,
        .indexCount = static_cast<uint32_t>(mesh.indices.size()),
    });
    return mesh;
}

static void LoadNodeRecursive(
    const tinygltf::Model& model,
    const tinygltf::Node& node,
    const glm::mat4& parentMatrix,
    AssetServer& assetServer,
    ModelAsset& outModel) {
    const glm::mat4 worldMatrix = parentMatrix * ComposeNodeMatrix(node);

    ModelNodeAsset outNode{};
    outNode.modelMatrix = worldMatrix;

    if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = model.meshes[static_cast<std::size_t>(node.mesh)];
        for (std::size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex) {
            const tinygltf::Primitive& primitive = mesh.primitives[primitiveIndex];
            MaterialAsset materialAsset{};
            materialAsset.shadingModel = EMaterialShadingModel::Lambert;

            if (primitive.material >= 0) {
                const tinygltf::Material& material = model.materials[static_cast<std::size_t>(primitive.material)];
                const auto& pbr = material.pbrMetallicRoughness;
                if (pbr.baseColorFactor.size() == 4U) {
                    materialAsset.baseColorFactor = glm::vec4{
                        static_cast<float>(pbr.baseColorFactor[0]),
                        static_cast<float>(pbr.baseColorFactor[1]),
                        static_cast<float>(pbr.baseColorFactor[2]),
                        static_cast<float>(pbr.baseColorFactor[3]),
                    };
                }

                if (pbr.baseColorTexture.index >= 0) {
                    materialAsset.baseColorTexture = LoadTextureAsset(
                        model,
                        pbr.baseColorTexture.index,
                        pbr.baseColorTexture.texCoord,
                        assetServer,
                        EImageAssetFormat::Rgba8Srgb);
                }
                materialAsset.isAlphaMasked = IsMaskedAlphaMode(material.alphaMode);
                materialAsset.metallicFactor = static_cast<float>(pbr.metallicFactor);
                materialAsset.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
                if (pbr.metallicRoughnessTexture.index >= 0) {
                    materialAsset.metallicRoughnessTexture = LoadTextureAsset(
                        model,
                        pbr.metallicRoughnessTexture.index,
                        pbr.metallicRoughnessTexture.texCoord,
                        assetServer,
                        EImageAssetFormat::Rgba8Unorm);
                }
                if (material.normalTexture.index >= 0) {
                    materialAsset.normalTexture = LoadTextureAsset(
                        model,
                        material.normalTexture.index,
                        material.normalTexture.texCoord,
                        assetServer,
                        EImageAssetFormat::Rgba8Unorm);
                    materialAsset.normalScale = static_cast<float>(material.normalTexture.scale);
                }
                materialAsset.specular = ReadSpecularExtension(model, material, assetServer);
                materialAsset.doubleSided = material.doubleSided;
                if (material.extensions.contains("KHR_materials_unlit")) {
                    materialAsset.shadingModel = EMaterialShadingModel::Unlit;
                }
            }

            MeshAsset meshAsset = BuildMesh(model, primitive);
            if (meshAsset.vertices.empty() || meshAsset.indices.empty()) {
                continue;
            }

            const AssetId<MeshAsset> meshId = assetServer.CreateMesh(std::move(meshAsset));
            materialAsset.useVertexColor = true;
            const AssetId<MaterialAsset> materialId = assetServer.CreateMaterial(std::move(materialAsset));

            outNode.primitives.push_back(ModelPrimitiveAsset{
                .mesh = meshId,
                .material = materialId,
                .primitiveIndex = static_cast<uint32_t>(primitiveIndex),
            });
        }
    }

    if (!outNode.primitives.empty()) {
        outModel.nodes.push_back(std::move(outNode));
    }

    for (const int childIndex : node.children) {
        if (childIndex < 0 || static_cast<std::size_t>(childIndex) >= model.nodes.size()) {
            continue;
        }
        LoadNodeRecursive(model, model.nodes[static_cast<std::size_t>(childIndex)], worldMatrix, assetServer, outModel);
    }
}

bool GltfImporter::Import(const std::filesystem::path& path, AssetServer& assetServer, ModelAsset& outModel) {
    tinygltf::TinyGLTF loader{};
    tinygltf::Model model{};
    std::string warn{};
    std::string err{};

    const bool loaded = path.extension() == ".glb"
                            ? loader.LoadBinaryFromFile(&model, &err, &warn, path.string())
                            : loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    if (!warn.empty()) {
        std::cerr << "[GltfImporter] " << warn;
        if (!warn.ends_with('\n')) {
            std::cerr << '\n';
        }
    }
    if (!loaded) {
        std::cerr << "[GltfImporter] Failed to load '" << path.string() << "': " << err << std::endl;
        outModel = ModelAsset{};
        return false;
    }
    WarnMaskedAlphaMode(model, path);
    WarnUnsupportedExtensions(model, path);

    outModel = ModelAsset{};
    if (model.scenes.empty()) {
        std::cerr << "[GltfImporter] No scenes in '" << path.string() << "'." << std::endl;
        return false;
    }

    const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (sceneIndex < 0 || static_cast<std::size_t>(sceneIndex) >= model.scenes.size()) {
        std::cerr << "[GltfImporter] Invalid scene index in '" << path.string() << "'." << std::endl;
        return false;
    }

    const tinygltf::Scene& scene = model.scenes[static_cast<std::size_t>(sceneIndex)];
    for (const int nodeIndex : scene.nodes) {
        if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= model.nodes.size()) {
            continue;
        }
        LoadNodeRecursive(model, model.nodes[static_cast<std::size_t>(nodeIndex)], glm::mat4{1.0f}, assetServer, outModel);
    }

    return !outModel.nodes.empty();
}
