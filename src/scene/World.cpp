#include "World.h"

#include "Entity.h"
#include "components/NameComponent.h"

Entity World::CreateEntity(const std::string_view name) {
    const entt::entity entity = m_registry.create();
    Entity             handle{entity, this};
    if (!name.empty()) {
        NameComponent& nameComponent = handle.AddComponent<NameComponent>();
        nameComponent.name = std::string{name};
    }
    return handle;
}

void World::DestroyEntity(const Entity entity) {
    if (!entity.IsValid()) {
        return;
    }
    if (entity.Handle() == m_primaryCamera) {
        m_primaryCamera = entt::null;
    }
    m_registry.destroy(entity.Handle());
}

entt::registry& World::Registry() {
    return m_registry;
}

const entt::registry& World::Registry() const {
    return m_registry;
}

void World::SetPrimaryCamera(const Entity entity) {
    m_primaryCamera = entity.IsValid() ? entity.Handle() : entt::null;
}

Entity World::PrimaryCamera() const {
    if (m_primaryCamera == entt::null || !m_registry.valid(m_primaryCamera)) {
        return {};
    }
    return {m_primaryCamera, const_cast<World*>(this)};
}

void World::Update(const float dt) {
    (void)dt;
}
