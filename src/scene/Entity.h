#ifndef BJTU_WGPU_RENDERER_ENTITY_H
#define BJTU_WGPU_RENDERER_ENTITY_H

#include <cassert>
#include <utility>

#include <entity/registry.hpp>

#include "scene/World.h"

class Entity {
public:
    Entity() = default;

    Entity(entt::entity handle, World* world)
        : m_handle(handle),
          m_world(world) {
    }

    template <typename Component, typename... Args>
    Component& AddComponent(Args&&... args) const {
        assert(m_world != nullptr);
        return m_world->Registry().emplace<Component>(m_handle, std::forward<Args>(args)...);
    }

    template <typename Component>
    [[nodiscard]] Component& GetComponent() const {
        assert(m_world != nullptr);
        return m_world->Registry().get<Component>(m_handle);
    }

    template <typename Component>
    [[nodiscard]] bool HasComponent() const {
        return m_world != nullptr && m_world->Registry().all_of<Component>(m_handle);
    }

    [[nodiscard]] bool IsValid() const {
        return m_world != nullptr && m_handle != entt::null && m_world->Registry().valid(m_handle);
    }

    [[nodiscard]] entt::entity Handle() const {
        return m_handle;
    }

    explicit operator bool() const {
        return IsValid();
    }

    bool operator==(const Entity& other) const = default;

private:
    entt::entity m_handle = entt::null;
    World*       m_world  = nullptr;
};

#endif // BJTU_WGPU_RENDERER_ENTITY_H
