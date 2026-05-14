#include "LogicScene.h"

#include <algorithm>
#include <memory>

#include "asset/types/MaterialAsset.h"
#include "asset/types/MeshAsset.h"
#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/StaticMeshComponent.h"
#include "components/TransformComponent.h"
#include "input/InputEventBus.h"
#include "render/RenderContext.h"
#include "render/legacy/LegacyGuiRenderer.h"
#include "scene/camera/FreeCameraController.h"
#include "scene/camera/PerspectiveCamera.h"

static MeshAsset CreateSolidCubeMeshAsset() {
    MeshAsset mesh{};
    constexpr glm::vec3 backColor{0.95f, 0.35f, 0.25f};
    constexpr glm::vec3 frontColor{0.20f, 0.65f, 0.95f};
    constexpr glm::vec3 bottomColor{0.95f, 0.85f, 0.25f};
    constexpr glm::vec3 topColor{0.35f, 0.85f, 0.45f};
    constexpr glm::vec3 leftColor{0.70f, 0.45f, 0.95f};
    constexpr glm::vec3 rightColor{0.95f, 0.55f, 0.80f};
    mesh.vertices = {
        {{-0.5f, -0.5f, -0.5f}, backColor},
        {{0.5f, -0.5f, -0.5f}, backColor},
        {{0.5f, 0.5f, -0.5f}, backColor},
        {{-0.5f, 0.5f, -0.5f}, backColor},
        {{-0.5f, -0.5f, 0.5f}, frontColor},
        {{0.5f, -0.5f, 0.5f}, frontColor},
        {{0.5f, 0.5f, 0.5f}, frontColor},
        {{-0.5f, 0.5f, 0.5f}, frontColor},
        {{-0.5f, -0.5f, -0.5f}, bottomColor},
        {{0.5f, -0.5f, -0.5f}, bottomColor},
        {{0.5f, -0.5f, 0.5f}, bottomColor},
        {{-0.5f, -0.5f, 0.5f}, bottomColor},
        {{-0.5f, 0.5f, -0.5f}, topColor},
        {{0.5f, 0.5f, -0.5f}, topColor},
        {{0.5f, 0.5f, 0.5f}, topColor},
        {{-0.5f, 0.5f, 0.5f}, topColor},
        {{-0.5f, -0.5f, -0.5f}, leftColor},
        {{-0.5f, 0.5f, -0.5f}, leftColor},
        {{-0.5f, 0.5f, 0.5f}, leftColor},
        {{-0.5f, -0.5f, 0.5f}, leftColor},
        {{0.5f, -0.5f, -0.5f}, rightColor},
        {{0.5f, 0.5f, -0.5f}, rightColor},
        {{0.5f, 0.5f, 0.5f}, rightColor},
        {{0.5f, -0.5f, 0.5f}, rightColor},
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

static MaterialAsset CreateDefaultMaterialAsset() {
    return MaterialAsset{};
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
        glm::vec3{1.8f, 1.5f, 2.4f},
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 1.0f, 0.0f});
    m_world.SetPrimaryCamera(cameraEntity);

    Entity cubeEntity = m_world.CreateEntity("Debug Cube");
    auto&  transform = cubeEntity.AddComponent<TransformComponent>();
    transform.transform.Combine(Transform3D::Scale(0.8f, 0.8f, 0.8f));
    auto& meshComponent = cubeEntity.AddComponent<StaticMeshComponent>();
    meshComponent.mesh = m_assetServer.CreateMesh(CreateSolidCubeMeshAsset());
    meshComponent.material = m_assetServer.CreateMaterial(CreateDefaultMaterialAsset());
    meshComponent.renderMode = Object3D::ERenderMode::Solid;

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
    // TODO：后续接入鼠标视角事件分发后，在这里把 CameraLookInputEvent 转给当前相机控制器。
}

void LogicScene::UnregisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<CameraMoveInputEvent>().disconnect<&LogicScene::OnCameraMoveInputEvent>(*this);
    eventBus.Dispatcher().sink<CameraLookInputEvent>().disconnect<&LogicScene::OnCameraLookInputEvent>(*this);
    // TODO：后续实现鼠标视角控制时，保持这里和 RegisterInputHandlers 中的订阅成对维护。
}

void LogicScene::OnCameraMoveInputEvent(const CameraMoveInputEvent& event) {
    if (m_cameraController != nullptr) {
        m_cameraController->OnMoveInput(event);
    }
}

void LogicScene::OnCameraLookInputEvent(const CameraLookInputEvent& event) {
    (void)event;
    // TODO：后续在这里把鼠标驱动的视角输入转发给相机控制器。
}

RenderScene LogicScene::BuildRenderScene(const RenderContext& ctx) const {
    RenderScene renderScene{};

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
            };
        }
    }

    auto view = m_world.View<TransformComponent, StaticMeshComponent>();
    renderScene.objects.reserve(view.size_hint());
    for (const entt::entity entityHandle : view) {
        const TransformComponent& transform = view.get<TransformComponent>(entityHandle);
        const StaticMeshComponent& mesh = view.get<StaticMeshComponent>(entityHandle);
        renderScene.objects.push_back(RenderObject{
            .worldMatrix = transform.transform.Matrix(),
            .meshId = mesh.mesh.id,
            .mesh = mesh.mesh.asset,
            .material = mesh.material.asset,
            .renderMode = mesh.renderMode,
        });
    }

    return renderScene;
}
