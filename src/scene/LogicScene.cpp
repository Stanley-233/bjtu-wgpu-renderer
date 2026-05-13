#include "LogicScene.h"

#include <algorithm>
#include <memory>

#include <glm/geometric.hpp>

#include "Entity.h"
#include "components/CameraComponent.h"
#include "components/StaticMeshComponent.h"
#include "components/TransformComponent.h"
#include "input/InputEventBus.h"
#include "render/RenderContext.h"
#include "render/legacy/LegacyGuiRenderer.h"
#include "scene/camera/PerspectiveCamera.h"

static LegacyMeshData3D CreateSolidCubeMesh() {
    LegacyMeshData3D mesh{};
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
    return mesh;
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

void LogicScene::Initialize(RenderContext& ctx) {
    m_renderer.Initialize(ctx);

    Entity cameraEntity = m_world.CreateEntity("Primary Camera");
    (void)cameraEntity.AddComponent<TransformComponent>();
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
    meshComponent.mesh = CreateSolidCubeMesh();
    meshComponent.renderMode = Object3D::ERenderMode::Solid;
}

void LogicScene::Update(const float dt) {
    constexpr float kEpsilon = 1e-6f;

    Entity primaryCameraEntity = FindPrimaryCamera(m_world);
    if (dt > 0.0f && primaryCameraEntity && primaryCameraEntity.HasComponent<CameraComponent>()) {
        CameraComponent& cameraComponent = primaryCameraEntity.GetComponent<CameraComponent>();
        if (cameraComponent.camera != nullptr) {
            const glm::vec3 position = cameraComponent.camera->Position();
            const glm::vec3 target = cameraComponent.camera->Target();
            const glm::vec3 up = cameraComponent.camera->Up();
            const glm::vec3 forwardRaw = target - position;
            const float forwardLen = glm::length(forwardRaw);
            if (forwardLen > kEpsilon) {
                const glm::vec3 forward = forwardRaw / forwardLen;
                glm::vec3 rightRaw = glm::cross(forward, up);
                const float rightLen = glm::length(rightRaw);
                if (rightLen > kEpsilon) {
                    rightRaw /= rightLen;
                    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
                    glm::vec3 moveDirection = forward * m_moveForward
                                              + rightRaw * m_moveRight
                                              + worldUp * m_moveUp;
                    const float moveDirectionLen = glm::length(moveDirection);
                    if (moveDirectionLen > kEpsilon) {
                        moveDirection /= moveDirectionLen;
                        const glm::vec3 delta = moveDirection * (kCameraMoveSpeed * dt);
                        cameraComponent.camera->SetPose(position + delta, target + delta, up);
                    }
                }
            }
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
}

void LogicScene::UnregisterInputHandlers(InputEventBus& eventBus) {
    eventBus.Dispatcher().sink<CameraMoveInputEvent>().disconnect<&LogicScene::OnCameraMoveInputEvent>(*this);
}

void LogicScene::OnCameraMoveInputEvent(const CameraMoveInputEvent& event) {
    m_moveForward = event.forward;
    m_moveRight = event.right;
    m_moveUp = event.up;
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
            .mesh = &mesh.mesh,
            .renderMode = mesh.renderMode,
        });
    }

    return renderScene;
}
