#include "World.h"

#include <algorithm>
#include <vector>

#include "Entity.h"
#include "components/ChildrenComponent.h"
#include "components/NameComponent.h"
#include "components/ParentComponent.h"
#include "components/TransformComponent.h"

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
    DestroyEntityRecursive(entity.Handle());
}

void World::DestroyEntityRecursive(const entt::entity entity) {
    if (!m_registry.valid(entity)) {
        return;
    }
    if (entity == m_primaryCamera) {
        m_primaryCamera = entt::null;
    }
    if (entity == m_directionalLight) {
        m_directionalLight = entt::null;
    }
    // 先复制 children，避免递归 destroy 时修改 vector 导致迭代器失效
    std::vector<entt::entity> childrenToDestroy;
    if (m_registry.all_of<ChildrenComponent>(entity)) {
        childrenToDestroy = m_registry.get<ChildrenComponent>(entity).children;
    }
    for (const entt::entity child : childrenToDestroy) {
        DestroyEntityRecursive(child);
    }
    // 从父节点的 children 里移除自己
    if (m_registry.all_of<ParentComponent>(entity)) {
        const entt::entity parent = m_registry.get<ParentComponent>(entity).parent;

        if (m_registry.valid(parent) && m_registry.all_of<ChildrenComponent>(parent)) {
            auto& siblings = m_registry.get<ChildrenComponent>(parent).children;
            std::erase(siblings, entity);
        }
    }
    m_registry.destroy(entity);
}

entt::registry& World::Registry() {
    return m_registry;
}

const entt::registry& World::Registry() const {
    return m_registry;
}

void World::SetParent(const Entity child, const Entity parent) {
    if (!child.IsValid() || !parent.IsValid()) {
        return;
    }
    if (child.Handle() == parent.Handle()) {
        return;
    }
    // 防止形成环：不能把父节点挂到自己的子孙下面
    for (entt::entity current = parent.Handle(); current != entt::null && m_registry.valid(current);) {
        if (current == child.Handle()) {
            return;
        }
        if (!m_registry.all_of<ParentComponent>(current)) {
            break;
        }
        current = m_registry.get<ParentComponent>(current).parent;
    }
    ClearParent(child);
    m_registry.emplace_or_replace<ParentComponent>(
        child.Handle(),
        parent.Handle()
    );
    auto& children = m_registry.get_or_emplace<ChildrenComponent>(
        parent.Handle()
    ).children;
    if (std::ranges::find(children, child.Handle()) == children.end()) {
        children.push_back(child.Handle());
    }
}

void World::ClearParent(const Entity child) {
    if (!child.IsValid()) {
        return;
    }
    if (!m_registry.all_of<ParentComponent>(child.Handle())) {
        return;
    }
    const entt::entity oldParent =
        m_registry.get<ParentComponent>(child.Handle()).parent;
    if (m_registry.valid(oldParent) && m_registry.all_of<ChildrenComponent>(oldParent)) {
        auto& children = m_registry.get<ChildrenComponent>(oldParent).children;
        std::erase(children, child.Handle());
    }
    m_registry.remove<ParentComponent>(child.Handle());
}

Entity World::ParentOf(const Entity child) const {
    if (!child.IsValid()) {
        return {};
    }
    if (!m_registry.all_of<ParentComponent>(child.Handle())) {
        return {};
    }
    const entt::entity parent =
        m_registry.get<ParentComponent>(child.Handle()).parent;
    if (parent == entt::null || !m_registry.valid(parent)) {
        return {};
    }
    return {parent, const_cast<World*>(this)};
}

std::vector<Entity> World::ChildrenOf(const Entity parent) const {
    std::vector<Entity> result;
    if (!parent.IsValid()) {
        return result;
    }
    if (!m_registry.all_of<ChildrenComponent>(parent.Handle())) {
        return result;
    }
    const auto& children = m_registry.get<ChildrenComponent>(parent.Handle()).children;
    result.reserve(children.size());
    for (const entt::entity child : children) {
        if (child != entt::null && m_registry.valid(child)) {
            result.emplace_back(child, const_cast<World*>(this));
        }
    }
    return result;
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

void World::SetDirectionalLight(const Entity entity) {
    // TODO: 后续在这里补唯一方向光的注册与校验逻辑
    m_directionalLight = entity.IsValid() ? entity.Handle() : entt::null;
}

Entity World::DirectionalLight() const {
    // TODO: 后续在这里补唯一方向光缺省发现逻辑
    if (m_directionalLight == entt::null || !m_registry.valid(m_directionalLight)) {
        return {};
    }
    return {m_directionalLight, const_cast<World*>(this)};
}

void World::Update(const float dt) {
    (void)dt;
}

glm::mat4 World::WorldMatrixOf(const Entity entity) const {
    if (!entity.IsValid()) {
        return glm::mat4(1.0f);
    }
    glm::mat4 localMatrix(1.0f);
    if (m_registry.all_of<TransformComponent>(entity.Handle())) {
        const auto& transform = m_registry.get<TransformComponent>(entity.Handle());
        localMatrix = transform.transform.Matrix();
    }
    if (!m_registry.all_of<ParentComponent>(entity.Handle())) {
        return localMatrix;
    }
    const entt::entity parent = m_registry.get<ParentComponent>(entity.Handle()).parent;
    if (parent == entt::null || !m_registry.valid(parent)) {
        return localMatrix;
    }
    return WorldMatrixOf(Entity{parent, const_cast<World*>(this)}) * localMatrix;
}
