#include "LegacyTomlSceneLoader.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

#include <toml.hpp>

#include "resource/loaders/ObjLoader.h"

static void Warn(const std::string& message) {
    std::cerr << "[TomlSceneLoader] " << message << '\n';
}

static void Error(const std::string& message) {
    std::cerr << "[TomlSceneLoader] " << message << '\n';
}

static bool TryReadVec3(
    const toml::table& table,
    const char*        key,
    glm::vec3&         out,
    const std::string& context
) {
    const toml::node_view<const toml::node> node = table[key];
    if (!node) {
        Warn(context + " missing '" + key + "', using default.");
        return false;
    }

    const toml::array* arr = node.as_array();
    if (arr == nullptr || arr->size() != 3U) {
        Warn(context + " field '" + key + "' must be [x, y, z], using default.");
        return false;
    }

    glm::vec3 parsed{};
    for (glm::vec3::length_type i = 0; i < 3; ++i) {
        const toml::node* elem = arr->get(static_cast<size_t>(i));
        if (elem == nullptr) {
            Warn(context + " field '" + key + "' has missing element, using default.");
            return false;
        }
        const auto value = elem->value<double>();
        if (!value.has_value()) {
            Warn(context + " field '" + key + "' contains non-numeric element, using default.");
            return false;
        }
        parsed[i] = static_cast<float>(*value);
    }
    out = parsed;
    return true;
}

static bool TryReadString(
    const toml::table& table,
    const char*        key,
    std::string&       out,
    const std::string& context
) {
    const toml::node_view<const toml::node> node = table[key];
    if (!node) {
        return false;
    }
    const auto value = node.value<std::string>();
    if (!value.has_value()) {
        Warn(context + " field '" + key + "' must be string, using default.");
        return false;
    }
    out = *value;
    return true;
}

static bool ParseProjectionType(const toml::node* projectionNode, ECameraProjectionType& outType) {
    if (projectionNode == nullptr) {
        Warn("camera.projectionType missing, using default Perspective.");
        outType = ECameraProjectionType::Perspective;
        return false;
    }

    const auto value = projectionNode->value<std::string>();
    if (!value.has_value()) {
        Warn("camera.projectionType must be string, using default Perspective.");
        outType = ECameraProjectionType::Perspective;
        return false;
    }

    if (*value == "Perspective") {
        outType = ECameraProjectionType::Perspective;
        return true;
    }
    if (*value == "Orthographic") {
        outType = ECameraProjectionType::Orthographic;
        return true;
    }

    Warn("camera.projectionType '" + *value + "' is unknown, fallback to Perspective.");
    outType = ECameraProjectionType::Perspective;
    return false;
}

static bool TryParseVertex(const toml::node& node, Vertex3D& outVertex, const std::string& context) {
    const toml::table* vertexTable = node.as_table();
    if (vertexTable == nullptr) {
        Warn(context + " is not a table, skipping vertex.");
        return false;
    }

    Vertex3D vertex{};
    bool     positionOk = TryReadVec3(*vertexTable, "position", vertex.position, context);
    bool     colorOk    = TryReadVec3(*vertexTable, "color", vertex.color, context);
    if (!positionOk || !colorOk) {
        Warn(context + " invalid position/color, skipping vertex.");
        return false;
    }

    outVertex = vertex;
    return true;
}

static bool TryParseIndices(
    const toml::array& indicesArray,
    const size_t       vertexCount,
    std::vector<uint16_t>& outIndices,
    bool&              outHasRangeError,
    const std::string& context
) {
    bool success = true;
    for (size_t i = 0; i < indicesArray.size(); ++i) {
        const toml::node* indexNode = indicesArray.get(i);
        if (indexNode == nullptr) {
            Warn(context + ".indices[" + std::to_string(i) + "] missing, skipping.");
            success = false;
            continue;
        }

        const auto idxValue = indexNode->value<int64_t>();
        if (!idxValue.has_value()) {
            Warn(context + ".indices[" + std::to_string(i) + "] is not integer, skipping.");
            success = false;
            continue;
        }
        if (*idxValue < 0) {
            Warn(context + ".indices[" + std::to_string(i) + "] is negative, skipping.");
            success = false;
            continue;
        }

        const size_t idx = static_cast<size_t>(*idxValue);
        if (idx >= vertexCount) {
            Error(context + ".indices[" + std::to_string(i) + "] out of range.");
            outHasRangeError = true;
            return false;
        }
        if (idx > static_cast<size_t>(std::numeric_limits<uint16_t>::max())) {
            Error(context + ".indices[" + std::to_string(i) + "] exceeds uint16_t range.");
            outHasRangeError = true;
            return false;
        }
        outIndices.push_back(static_cast<uint16_t>(idx));
    }
    return success;
}

static bool TryParseInlineMesh(
    const toml::table& meshTable,
    LegacyMeshData3D&        outMesh,
    const std::string& context
) {
    outMesh.vertices.clear();
    outMesh.indices.clear();

    const toml::node_view<const toml::node> verticesNode = meshTable["vertices"];
    if (!verticesNode) {
        Warn(context + ".vertices missing, object keeps empty mesh.");
    } else {
        const toml::array* verticesArray = verticesNode.as_array();
        if (verticesArray == nullptr) {
            Warn(context + ".vertices is not an array, object keeps empty mesh.");
        } else {
            for (size_t vIdx = 0; vIdx < verticesArray->size(); ++vIdx) {
                const toml::node* vertexNode = verticesArray->get(vIdx);
                if (vertexNode == nullptr) {
                    Warn(context + ".vertices[" + std::to_string(vIdx) + "] missing, skipping.");
                    continue;
                }

                Vertex3D vertex{};
                if (TryParseVertex(*vertexNode, vertex, context + ".vertices[" + std::to_string(vIdx) + "]")) {
                    outMesh.vertices.push_back(vertex);
                }
            }
        }
    }

    if (outMesh.vertices.empty()) {
        Warn(context + " parsed with zero valid vertices.");
    }

    const toml::node_view<const toml::node> indicesNode = meshTable["indices"];
    if (!indicesNode) {
        Warn(context + ".indices missing, using empty indices.");
    } else {
        const toml::array* indicesArray = indicesNode.as_array();
        if (indicesArray == nullptr) {
            Warn(context + ".indices is not an array, using empty indices.");
        } else {
            bool rangeError = false;
            TryParseIndices(*indicesArray, outMesh.vertices.size(), outMesh.indices, rangeError, context);
            if (rangeError) {
                outMesh.vertices.clear();
                outMesh.indices.clear();
                return false;
            }
        }
    }

    return true;
}

static bool TryApplyTransformTable(
    const toml::table&  transformTable,
    ObjectDescription&  outObject,
    const std::string&  context
) {
    TryReadVec3(transformTable, "translation", outObject.translation, context);
    TryReadVec3(transformTable, "rotation", outObject.rotation, context);
    TryReadVec3(transformTable, "scale", outObject.scale, context);
    return true;
}

static bool TryLoadObjMesh(
    const std::filesystem::path& scenePath,
    const std::string&           objPathRaw,
    LegacyMeshData3D&                  outMesh,
    const std::string&           context
) {
    if (objPathRaw.empty()) {
        return false;
    }

    ObjLoader loader{};
    const std::filesystem::path rawPath = objPathRaw;
    if (rawPath.is_relative()) {
        const std::filesystem::path resolved = scenePath.parent_path() / rawPath;
        if (loader.Load(resolved, outMesh)) {
            return true;
        }
    }

    if (loader.Load(rawPath, outMesh)) {
        return true;
    }

    Warn(context + ".objPath failed to load '" + objPathRaw + "', skipping object.");
    return false;
}

bool LegacyTomlSceneLoader::Load(const std::filesystem::path& path, SceneDescription& outScene) {
    outScene = SceneDescription{};

    SceneDescription parsedScene{};

    toml::table root;
    try {
        root = toml::parse_file(path.string());
    } catch (const toml::parse_error& err) {
        Error(std::string("failed to parse TOML: ") + std::string(err.description()));
        return false;
    } catch (const std::exception& err) {
        Error(std::string("failed to read scene file: ") + err.what());
        return false;
    }

    const auto cameraNode = root["camera"];
    if (!cameraNode) {
        Warn("camera section missing, using defaults.");
    } else {
        const toml::table* cameraTable = cameraNode.as_table();
        if (cameraTable == nullptr) {
            Warn("camera section is not a table, using defaults.");
        } else {
            ParseProjectionType(cameraTable->get("projectionType"), parsedScene.camera.projectionType);
            TryReadVec3(*cameraTable, "position", parsedScene.camera.position, "camera");
            TryReadVec3(*cameraTable, "target", parsedScene.camera.target, "camera");
            TryReadVec3(*cameraTable, "up", parsedScene.camera.up, "camera");
        }
    }

    const auto objectsNode = root["objects"];
    if (!objectsNode) {
        Warn("objects section missing, scene will contain zero objects.");
        outScene = parsedScene;
        return true;
    }

    const toml::array* objectsArray = objectsNode.as_array();
    if (objectsArray == nullptr) {
        Warn("objects section is not an array, scene will contain zero objects.");
        outScene = parsedScene;
        return true;
    }
    const toml::table& rootConst = root;

    for (size_t objIdx = 0; objIdx < objectsArray->size(); ++objIdx) {
        const std::string objectContext = "objects[" + std::to_string(objIdx) + "]";

        const toml::node* objectNode = objectsArray->get(objIdx);
        if (objectNode == nullptr) {
            Warn(objectContext + " missing, skipping object.");
            continue;
        }

        const toml::table* objectTable = objectNode->as_table();
        if (objectTable == nullptr) {
            Warn(objectContext + " is not a table, skipping object.");
            continue;
        }

        ObjectDescription object{};
        TryReadString(*objectTable, "name", object.name, objectContext);

        bool hasInlineTransform = false;
        const toml::node_view<const toml::node> transformNode = (*objectTable)["transform"];
        if (transformNode) {
            const toml::table* transformTable = transformNode.as_table();
            if (transformTable == nullptr) {
                Warn(objectContext + ".transform is not a table, using defaults.");
            } else {
                hasInlineTransform = true;
                TryApplyTransformTable(*transformTable, object, objectContext + ".transform");
            }
        }

        if (!hasInlineTransform && !object.name.empty()) {
            const toml::node_view<const toml::node> namedObjectNode = rootConst[object.name];
            if (namedObjectNode) {
                const toml::table* namedObjectTable = namedObjectNode.as_table();
                if (namedObjectTable == nullptr) {
                    Warn(object.name + " is not a table, transform keeps defaults.");
                } else {
                    const toml::node_view<const toml::node> namedTransformNode = (*namedObjectTable)["transform"];
                    if (namedTransformNode) {
                        const toml::table* namedTransformTable = namedTransformNode.as_table();
                        if (namedTransformTable == nullptr) {
                            Warn(object.name + ".transform is not a table, using defaults.");
                        } else {
                            TryApplyTransformTable(*namedTransformTable, object, object.name + ".transform");
                        }
                    }
                }
            }
        }

        const toml::table* meshTable = nullptr;
        if (!object.name.empty()) {
            const toml::node_view<const toml::node> namedObjectNode = rootConst[object.name];
            if (namedObjectNode) {
                const toml::table* namedObjectTable = namedObjectNode.as_table();
                if (namedObjectTable == nullptr) {
                    Warn(object.name + " is not a table, skipping object.");
                    continue;
                }
                const toml::node_view<const toml::node> namedMeshNode = (*namedObjectTable)["mesh"];
                if (namedMeshNode) {
                    meshTable = namedMeshNode.as_table();
                    if (meshTable == nullptr) {
                        Warn(object.name + ".mesh is not a table, skipping object.");
                        continue;
                    }
                }
            }
        }

        if (meshTable == nullptr) {
            Warn(objectContext + " mesh missing in [" + object.name + ".mesh], skipping object.");
            continue;
        }

        std::string objPathRaw{};
        (void)TryReadString(*meshTable, "objPath", objPathRaw, objectContext + ".mesh");

        bool loadedMesh = false;
        if (!objPathRaw.empty()) {
            loadedMesh = TryLoadObjMesh(path, objPathRaw, object.mesh, objectContext + ".mesh");
        } else {
            loadedMesh = TryParseInlineMesh(*meshTable, object.mesh, objectContext + ".mesh");
        }

        if (!loadedMesh || object.mesh.vertices.empty() || object.mesh.indices.empty()) {
            Warn(objectContext + " mesh is empty or failed to load, skipping object.");
            continue;
        }

        parsedScene.objects.push_back(std::move(object));
    }

    outScene = std::move(parsedScene);
    return true;
}
