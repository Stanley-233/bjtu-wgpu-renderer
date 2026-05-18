#ifndef BJTU_WGPU_RENDERER_WORLD_H
#define BJTU_WGPU_RENDERER_WORLD_H

#include <string_view>

#include <entity/registry.hpp>
#include <glm/mat4x4.hpp>

class Entity;

class World {
public:
    Entity CreateEntity(std::string_view name = {});

    void DestroyEntity(Entity entity);

    [[nodiscard]] entt::registry& Registry();

    [[nodiscard]] const entt::registry& Registry() const;

    // 查询所有具备某些 Component 的 Entity
    template <typename... Components>
    [[nodiscard]] auto View() {
        return m_registry.view<Components...>();
    }

    template <typename... Components>
    [[nodiscard]] auto View() const {
        return m_registry.view<Components...>();
    }

    void SetParent(Entity child, Entity parent);

    void ClearParent(Entity child);

    [[nodiscard]] Entity ParentOf(Entity child) const;

    [[nodiscard]] std::vector<Entity> ChildrenOf(Entity parent) const;

    void SetPrimaryCamera(Entity entity);

    [[nodiscard]] Entity PrimaryCamera() const;

    void SetDirectionalLight(Entity entity);

    [[nodiscard]] Entity DirectionalLight() const;

    static void Update(float dt);

    [[nodiscard]] glm::mat4 WorldMatrixOf(Entity entity) const;

private:
    void DestroyEntityRecursive(entt::entity entity);

    entt::registry m_registry{};
    entt::entity   m_primaryCamera = entt::null;
    entt::entity   m_directionalLight = entt::null;
};

#endif // BJTU_WGPU_RENDERER_WORLD_H
