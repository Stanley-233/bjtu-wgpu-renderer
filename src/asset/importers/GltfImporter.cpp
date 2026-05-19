#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION

#include "GltfImporter.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
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

    if (accessorIndex < 0) {
        return fail("accessor index is negative");
    }

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

static ImageAsset DecodeImage(const tinygltf::Image& image) {
    ImageAsset asset{};
    asset.width = static_cast<uint32_t>(std::max(0, image.width));
    asset.height = static_cast<uint32_t>(std::max(0, image.height));

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
    std::vector<glm::vec2> texcoords{};
    std::vector<glm::vec4> colors{};
    const auto normalIt = primitive.attributes.find("NORMAL");
    if (normalIt != primitive.attributes.end()) {
        (void)ReadAccessorVec3(model, normalIt->second, normals);
    }
    const auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
    if (texcoordIt != primitive.attributes.end()) {
        (void)ReadAccessorVec2(model, texcoordIt->second, texcoords);
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
        if (vertexIndex < texcoords.size()) {
            mesh.vertices[vertexIndex].uv0 = texcoords[vertexIndex];
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
            glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
            AssetId<ImageAsset> baseColorTexture{};

            if (primitive.material >= 0) {
                const tinygltf::Material& material = model.materials[static_cast<std::size_t>(primitive.material)];
                const auto& pbr = material.pbrMetallicRoughness;
                if (pbr.baseColorFactor.size() == 4U) {
                    baseColorFactor = glm::vec4{
                        static_cast<float>(pbr.baseColorFactor[0]),
                        static_cast<float>(pbr.baseColorFactor[1]),
                        static_cast<float>(pbr.baseColorFactor[2]),
                        static_cast<float>(pbr.baseColorFactor[3]),
                    };
                }

                if (pbr.baseColorTexture.index >= 0) {
                    const int textureIndex = pbr.baseColorTexture.index;
                    if (textureIndex >= 0 && static_cast<std::size_t>(textureIndex) < model.textures.size()) {
                        const tinygltf::Texture& texture = model.textures[static_cast<std::size_t>(textureIndex)];
                        if (texture.source >= 0 && static_cast<std::size_t>(texture.source) < model.images.size()) {
                            const tinygltf::Image& image = model.images[static_cast<std::size_t>(texture.source)];
                            baseColorTexture = assetServer.CreateImage(DecodeImage(image));
                        }
                    }
                }
            }

            MeshAsset meshAsset = BuildMesh(model, primitive);
            if (meshAsset.vertices.empty() || meshAsset.indices.empty()) {
                continue;
            }

            const AssetId<MeshAsset> meshId = assetServer.CreateMesh(std::move(meshAsset));
            MaterialAsset materialAsset{};
            // TODO: glTF 材质默认应映射到 Lambert 或更完整的光照模型，仅 KHR_materials_unlit 应走 Unlit
            materialAsset.shadingModel = EMaterialShadingModel::Unlit;
            materialAsset.baseColorFactor = baseColorFactor;
            materialAsset.baseColorTexture = baseColorTexture;
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
