#include "LogicScene.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include "asset/AssetPaths.h"
#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"
#include "asset/types/ModelAsset.h"
#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/light/DirectionalLightComponent.h"
#include "components/StaticMeshComponent.h"
#include "components/TransformComponent.h"
#include "input/InputEventBus.h"
#include "render/RenderContext.h"
#include "scene/camera/FreeCameraController.h"
#include "scene/camera/PerspectiveCamera.h"

[[maybe_unused]] static MeshAsset CreateSolidCubeMeshAsset() {
    MeshAsset mesh{};
    constexpr glm::vec4 backColor{0.95f, 0.35f, 0.25f, 1.0f};
    constexpr glm::vec4 frontColor{0.20f, 0.65f, 0.95f, 1.0f};
    constexpr glm::vec4 bottomColor{0.95f, 0.85f, 0.25f, 1.0f};
    constexpr glm::vec4 topColor{0.35f, 0.85f, 0.45f, 1.0f};
    constexpr glm::vec4 leftColor{0.70f, 0.45f, 0.95f, 1.0f};
    constexpr glm::vec4 rightColor{0.95f, 0.55f, 0.80f, 1.0f};
    mesh.vertices = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, backColor},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, backColor},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, backColor},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, backColor},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, frontColor},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, frontColor},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, frontColor},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, frontColor},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, bottomColor},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, bottomColor},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, bottomColor},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, bottomColor},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, topColor},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, topColor},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, topColor},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, topColor},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, leftColor},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, leftColor},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, leftColor},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, leftColor},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, rightColor},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, rightColor},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, rightColor},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, rightColor},
    };
    mesh.indices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 22, 21, 20, 23, 22,
    };
    mesh.primitiveRanges.push_back(MeshPrimitiveRange{
        .firstIndex = 0,
        .indexCount = static_cast<uint32_t>(mesh.indices.size()),
    });
    return mesh;
}

[[maybe_unused]] static MaterialAsset CreateDefaultMaterialAsset() {
    return MaterialAsset{};
}

glm::vec3 NormalizeDirectionOrDefault(const glm::vec3& direction) {
    constexpr float kMinDirectionLength = 1.0e-4f;
    if (glm::length(direction) < kMinDirectionLength) {
        return {0.0f, -1.0f, 0.0f};
    }
    return glm::normalize(direction);
}

DirectionalShadowSceneData BuildDirectionalShadowSceneData(const glm::vec3& lightDirection) {
    DirectionalShadowSceneData shadowData{};
    (void)lightDirection;
    //TODO: [Shadow] 先根据 Directional Light 的方向构造 light view matrix
    //TODO: [Shadow] 再根据主相机可见范围或场景包围盒构造正交投影矩阵
    //TODO: [Shadow] 最后把 lightProjection * lightView 写入 shadowData.uniformData.lightViewProjection
    //TODO: [Shadow] 当 lightViewProjection 准备好后，再把 shadowData.uniformData.shadowParams.x 设为 1，表示启用阴影
    return shadowData;
}

Entity FindPrimaryCamera(World& world) {
    Entity primary = world.PrimaryCamera();
    if (primary && primary.HasComponent<CameraComponent>()) {
        return primary;
    }

    auto view = world.View<CameraComponent>();
    for (const entt::entity entityHandle : view) {
        const CameraComponent& camera = view.get<CameraComponent>(entityHandle);
        if (camera.isPrimary) {
            return {entityHandle, &world};
        }
    }
    return {};
}

void ApplyStaticMeshShadingModelOverride(
    const Entity entity,
    const std::optional<EMaterialShadingModel>& shadingModel) {
    if (!entity || !entity.HasComponent<StaticMeshComponent>()) {
        return;
    }

    auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
    if (shadingModel.has_value()) {
        meshComponent.SetShadingModelOverride(*shadingModel);
        return;
    }
    meshComponent.ClearShadingModelOverride();
}

LogicScene::LogicScene()
    : m_cameraController(std::make_unique<FreeCameraController>()) {
}

bool LogicScene::Initialize(RenderContext& renderCtx) {
    m_renderer.Initialize(renderCtx);

    Entity cameraEntity = m_world.CreateEntity("Primary Camera");
    CameraComponent& cameraComponent = cameraEntity.AddComponent<CameraComponent>();
    cameraComponent.camera = std::make_unique<PerspectiveCamera>();
    cameraComponent.isPrimary = true;
    if (auto* perspectiveCamera = dynamic_cast<PerspectiveCamera*>(cameraComponent.camera.get())) {
        perspectiveCamera->SetPerspective(1.0471976f, 0.1f, 100.0f);
    }
    ConfigureInitialCamera(cameraComponent);
    m_world.SetPrimaryCamera(cameraEntity);

    Entity directionalLightEntity = m_world.CreateEntity("Main Directional Light");
    auto&  directionalLight       = directionalLightEntity.AddComponent<DirectionalLightComponent>();
    ConfigureInitialDirectionalLight(directionalLight);
    directionalLight.direction = NormalizeDirectionOrDefault(directionalLight.direction);
    m_world.SetDirectionalLight(directionalLightEntity);

    return BuildSceneContent();
}

void LogicScene::Update(const float dt) {
    Entity primaryCameraEntity = FindPrimaryCamera(m_world);
    if (m_cameraController != nullptr && primaryCameraEntity && primaryCameraEntity.HasComponent<CameraComponent>()) {
        CameraComponent& cameraComponent = primaryCameraEntity.GetComponent<CameraComponent>();
        if (cameraComponent.camera != nullptr) {
            m_cameraController->Update(dt, *cameraComponent.camera);
        }
    }

    m_world.Update(dt);
}

void LogicScene::Render(RenderContext& renderCtx, LegacyGuiRenderer& guiRenderer) {
    RenderScene renderScene = BuildRenderScene(renderCtx);
    m_renderer.Render(renderCtx, renderScene, guiRenderer);
}

void LogicScene::RegisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<CameraMoveInputEvent>().connect<&LogicScene::OnCameraMoveInputEvent>(*this);
    eventBus.Dispatcher().sink<CameraLookInputEvent>().connect<&LogicScene::OnCameraLookInputEvent>(*this);
}

void LogicScene::UnregisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<CameraMoveInputEvent>().disconnect<&LogicScene::OnCameraMoveInputEvent>(*this);
    eventBus.Dispatcher().sink<CameraLookInputEvent>().disconnect<&LogicScene::OnCameraLookInputEvent>(*this);
}

void LogicScene::OnCameraMoveInputEvent(const CameraMoveInputEvent& event) {
    if (m_cameraController != nullptr) {
        m_cameraController->OnMoveInput(event);
    }
}

void LogicScene::OnCameraLookInputEvent(const CameraLookInputEvent& event) {
    if (m_cameraController != nullptr) {
        m_cameraController->OnLookInput(event);
    }
}

World& LogicScene::GetWorld() {
    return m_world;
}

const World& LogicScene::GetWorld() const {
    return m_world;
}

AssetServer& LogicScene::GetAssetServer() {
    return m_assetServer;
}

const AssetServer& LogicScene::GetAssetServer() const {
    return m_assetServer;
}

Entity LogicScene::LoadModelRoot(const std::filesystem::path& path, const std::string_view namePrefix) {
    return LoadModelRoot(path, namePrefix, ModelSpawnOptions{});
}

Entity LogicScene::LoadModelRoot(
    const std::filesystem::path& path,
    const std::string_view       namePrefix,
    ModelSpawnOptions            options) {
    const std::filesystem::path resolvedPath = path.is_absolute() ? path : AssetPaths::Resolve(path);
    const AssetId<ModelAsset> modelId = m_assetServer.LoadModel(resolvedPath);
    if (!modelId.IsValid()) {
        std::cerr << '[' << Name() << "] Failed to load " << resolvedPath.string() << std::endl;
        return {};
    }

    const ModelAsset* model = m_assetServer.Get(modelId);
    if (model == nullptr) {
        std::cerr << '[' << Name() << "] Loaded model id was invalid for " << resolvedPath.string() << std::endl;
        return {};
    }

    return SpawnModelEntities(*model, namePrefix, options);
}

Entity LogicScene::SpawnModelEntities(const ModelAsset& model, const std::string_view namePrefix) {
    return SpawnModelEntities(model, namePrefix, ModelSpawnOptions{});
}

Entity LogicScene::SpawnModelEntities(
    const ModelAsset&      model,
    const std::string_view namePrefix,
    ModelSpawnOptions      options) {
    const std::string rootName{namePrefix};
    Entity root = m_world.CreateEntity(rootName);
    (void)root.AddComponent<TransformComponent>();

    std::size_t nodeCounter = 0;
    for (const ModelNodeAsset& node : model.nodes) {
        for (const ModelPrimitiveAsset& primitive : node.primitives) {
            const MeshAsset* meshAsset = m_assetServer.Get(primitive.mesh);
            const MaterialAsset* materialAsset = m_assetServer.Get(primitive.material);
            if (meshAsset == nullptr || materialAsset == nullptr) {
                continue;
            }

            Entity mesh = m_world.CreateEntity(rootName + " " + std::to_string(nodeCounter++));
            mesh.SetParent(root);
            auto& transform = mesh.AddComponent<TransformComponent>();
            transform.transform.SetMatrix(node.modelMatrix);
            auto& meshComponent = mesh.AddComponent<StaticMeshComponent>();
            meshComponent.mesh = primitive.mesh;
            meshComponent.material = primitive.material;
            if (options.shadingModelOverride.has_value()) {
                meshComponent.SetShadingModelOverride(*options.shadingModelOverride);
            }
        }
    }
    return root;
}

void LogicScene::SetStaticMeshShadingModelOverride(
    const Entity entity,
    const std::optional<EMaterialShadingModel> shadingModel) {
    ApplyStaticMeshShadingModelOverride(entity, shadingModel);
}

void LogicScene::SetStaticMeshShadingModelOverrideRecursive(
    const Entity root,
    const std::optional<EMaterialShadingModel> shadingModel) {
    if (!root) {
        return;
    }

    ApplyStaticMeshShadingModelOverride(root, shadingModel);
    for (const Entity child : root.GetChildren()) {
        SetStaticMeshShadingModelOverrideRecursive(child, shadingModel);
    }
}

RenderScene LogicScene::BuildRenderScene(const RenderContext& renderCtx) const {
    RenderScene renderScene{};
    renderScene.assetServer = &m_assetServer;
    renderScene.lights = BuildRenderLightSet();
    if (renderScene.lights.directionalLightCount > 0) {
        renderScene.directionalShadow = BuildDirectionalShadowSceneData(glm::vec3{renderScene.lights.directionalLight.direction});
    }

    int surfaceWidth = 0;
    int surfaceHeight = 0;
    renderCtx.GetSurfaceSize(surfaceWidth, surfaceHeight);
    const float aspect = static_cast<float>(std::max(1, surfaceWidth)) / static_cast<float>(std::max(1, surfaceHeight));

    World& world = const_cast<World&>(m_world);
    Entity primaryCameraEntity = FindPrimaryCamera(world);
    if (primaryCameraEntity && primaryCameraEntity.HasComponent<CameraComponent>()) {
        const CameraComponent& cameraComponent = primaryCameraEntity.GetComponent<CameraComponent>();
        if (cameraComponent.camera != nullptr) {
            renderScene.camera = RenderCamera{
                .view = cameraComponent.camera->View(),
                .projection = cameraComponent.camera->Projection(aspect),
                .position = cameraComponent.camera->Position(),
            };
        }
    }

    auto view = m_world.View<TransformComponent, StaticMeshComponent>();
    renderScene.objects.reserve(view.size_hint());
    for (const entt::entity entityHandle : view) {
        const StaticMeshComponent& mesh = view.get<StaticMeshComponent>(entityHandle);
        const MaterialAsset* const materialAsset = m_assetServer.Get(mesh.material);
        renderScene.objects.push_back(RenderObject{
            .worldMatrix = m_world.WorldMatrixOf(Entity{entityHandle, const_cast<World*>(&m_world)}),
            .meshId = mesh.mesh,
            .materialId = mesh.material,
            .shadingModel = mesh.ResolveShadingModel(materialAsset),
        });
    }

    return renderScene;
}

RenderLightSet LogicScene::BuildRenderLightSet() const {
    RenderLightSet lightSet{};
    const Entity   directionalLightEntity = m_world.DirectionalLight();
    if (!directionalLightEntity || !directionalLightEntity.HasComponent<DirectionalLightComponent>()) {
        return lightSet;
    }
    const auto&     directionalLight    = directionalLightEntity.GetComponent<DirectionalLightComponent>();
    const glm::vec3 direction           = NormalizeDirectionOrDefault(directionalLight.direction);
    lightSet.directionalLight.direction = glm::vec4{direction, 0.0f};
    lightSet.directionalLight.color     = glm::vec4{
        directionalLight.color * directionalLight.intensity,
        1.0f,
    };
    lightSet.directionalLightCount = 1;
    // TODO: 从 World 收集 Point/Spot Light
    return lightSet;
}
