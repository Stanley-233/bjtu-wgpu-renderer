#ifndef BJTU_WGPU_RENDERER_WORLD_H
#define BJTU_WGPU_RENDERER_WORLD_H

#include <string_view>

#include <entity/registry.hpp>

class Entity;

class World {
public:
    Entity CreateEntity(std::string_view name = {});

    void DestroyEntity(Entity entity);

    [[nodiscard]] entt::registry& Registry();

    [[nodiscard]] const entt::registry& Registry() const;

    template <typename... Components>
    [[nodiscard]] auto View() {
        return m_registry.view<Components...>();
    }

    template <typename... Components>
    [[nodiscard]] auto View() const {
        return m_registry.view<Components...>();
    }

    void SetPrimaryCamera(Entity entity);

    [[nodiscard]] Entity PrimaryCamera() const;

    void Update(float dt);

private:
    entt::registry m_registry{};
    entt::entity   m_primaryCamera = entt::null;
};

#endif // BJTU_WGPU_RENDERER_WORLD_H
