#include "LogicScene.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <glm/geometric.hpp>

#include "asset/AssetPaths.h"
#include "asset/AssetId.h"
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

static Entity SpawnModelEntities(World&             world,
                                 const AssetServer& assetServer,
                                 const ModelAsset&  model,
                                 const std::string& namePrefix) {
    Entity root = world.CreateEntity(namePrefix);
    (void)root.AddComponent<TransformComponent>();

    std::size_t nodeCounter = 0;
    for (const ModelNodeAsset& node : model.nodes) {
        for (const ModelPrimitiveAsset& primitive : node.primitives) {
            const MeshAsset* meshAsset = assetServer.Get(primitive.mesh);
            const MaterialAsset* materialAsset = assetServer.Get(primitive.material);
            if (meshAsset == nullptr || materialAsset == nullptr) {
                continue;
            }
            Entity mesh = world.CreateEntity(namePrefix + " " + std::to_string(nodeCounter++));
            mesh.SetParent(root);
            auto& transform = mesh.AddComponent<TransformComponent>();
            transform.transform.SetMatrix(node.modelMatrix);
            auto& meshComponent = mesh.AddComponent<StaticMeshComponent>();
            meshComponent.mesh = primitive.mesh;
            meshComponent.material = primitive.material;
        }
    }
    return root;
}

LogicScene::LogicScene()
    : m_cameraController(std::make_unique<FreeCameraController>()) {
}

bool LogicScene::Initialize(RenderContext& ctx) {
    m_renderer.Initialize(ctx);

    Entity cameraEntity = m_world.CreateEntity("Primary Camera");
    CameraComponent& cameraComponent = cameraEntity.AddComponent<CameraComponent>();
    cameraComponent.camera = std::make_unique<PerspectiveCamera>();
    cameraComponent.isPrimary = true;
    if (auto* perspectiveCamera = dynamic_cast<PerspectiveCamera*>(cameraComponent.camera.get())) {
        perspectiveCamera->SetPerspective(1.0471976f, 0.1f, 100.0f);
    }
    cameraComponent.camera->SetPose(
        glm::vec3{0.0f, 0.0f, 3.0f},
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 1.0f, 0.0f});
    m_world.SetPrimaryCamera(cameraEntity);

    // 注册平行光
    Entity directionalLightEntity = m_world.CreateEntity("Main Directional Light");
    auto&  directionalLight       = directionalLightEntity.AddComponent<DirectionalLightComponent>();
    directionalLight.direction    = NormalizeDirectionOrDefault(glm::vec3{-0.35f, -1.0f, -0.25f});
    directionalLight.intensity    = 1.8f;
    directionalLight.color        = glm::vec3{1.0f, 1.0f, 1.0f};
    m_world.SetDirectionalLight(directionalLightEntity);

    /*
    const auto& simpleMeshesPath = AssetPaths::Resolve("gltf-test/SimpleMeshes.gltf");
    const AssetId<ModelAsset> simpleMeshes = m_assetServer.LoadModel(simpleMeshesPath);
    if (simpleMeshes.IsValid()) {
        const ModelAsset* model = m_assetServer.Get(simpleMeshes);
        if (model == nullptr) {
            std::cerr << "[LogicScene] Loaded model id was invalid for " << simpleMeshesPath.string() << std::endl;
            return false;
        }
        Entity simpleMeshesRoot = SpawnModelEntities(m_world, m_assetServer, *model, "SimpleMeshes");
        auto& [rootTransform] = simpleMeshesRoot.GetComponent<TransformComponent>();
        rootTransform.SetTranslation(-1.5f, 0.0f, 0.0f);
    } else {
        std::cerr << "[LogicScene] Failed to load " << simpleMeshesPath.string() << std::endl;
    }
    */

    // 小庄模型（二次元渲染最好用Unlit）
    const auto& arkZfyPath= AssetPaths::Resolve("arknights/zhuang_fangyi__arknights_endfield.glb");
    const AssetId<ModelAsset> arkZfy = m_assetServer.LoadModel(arkZfyPath);
    if (arkZfy.IsValid()) {
        const ModelAsset* model = m_assetServer.Get(arkZfy);
        if (model == nullptr) {
            std::cerr << "[LogicScene] Loaded model id was invalid for " << arkZfyPath.string() << std::endl;
            return false;
        }
        Entity simpleTextureRoot = SpawnModelEntities(m_world, m_assetServer, *model, "SimpleTexture");
        auto& [transform] = simpleTextureRoot.GetComponent<TransformComponent>();
        transform.SetTranslation(1.5f, 0.0f, 0.0f);
        transform = transform.RotationX(-3.14f / 2.0);
    } else {
        std::cerr << "[LogicScene] Failed to load " << arkZfyPath.string() << std::endl;
    }

    const auto& cornelBoxPath = AssetPaths::Resolve("DiningRoom_GLTF/DiningRoom.gltf");
    const AssetId<ModelAsset> cornelBox = m_assetServer.LoadModel(cornelBoxPath);
    if (cornelBox.IsValid()) {
        const ModelAsset* model = m_assetServer.Get(cornelBox);
        if (model == nullptr) {
            std::cerr << "[LogicScene] Loaded model id was invalid for " << cornelBoxPath.string() << std::endl;
            return false;
        }
        Entity simpleMeshesRoot = SpawnModelEntities(m_world, m_assetServer, *model, "cornelBox");
        auto& [rootTransform] = simpleMeshesRoot.GetComponent<TransformComponent>();
        rootTransform.SetTranslation(-1.5f, 0.0f, -3.0f);
    } else {
        std::cerr << "[LogicScene] Failed to load " << cornelBoxPath.string() << std::endl;
    }

    return true;
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

void LogicScene::Render(RenderContext& ctx, LegacyGuiRenderer& guiRenderer) {
    RenderScene renderScene = BuildRenderScene(ctx);
    m_renderer.Render(ctx, renderScene, guiRenderer);
}

const char* LogicScene::Name() const {
    return "LogicScene";
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

RenderScene LogicScene::BuildRenderScene(const RenderContext& ctx) const {
    RenderScene renderScene{};
    renderScene.assetServer = &m_assetServer;
    renderScene.lights = BuildRenderLightSet();

    int surfaceWidth = 0;
    int surfaceHeight = 0;
    ctx.GetSurfaceSize(surfaceWidth, surfaceHeight);
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
        renderScene.objects.push_back(RenderObject{
            .worldMatrix = m_world.WorldMatrixOf(Entity{entityHandle, const_cast<World*>(&m_world)}),
            .meshId = mesh.mesh,
            .materialId = mesh.material,
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
