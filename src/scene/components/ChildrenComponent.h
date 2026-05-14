//
// Created by Stanley on 2026/5/14.
//

#ifndef BJTU_WGPU_RENDERER_CHILDCOMPONENT_H
#define BJTU_WGPU_RENDERER_CHILDCOMPONENT_H

#include <vector>

#include <entity/entity.hpp>

struct ChildrenComponent {
    std::vector<entt::entity> children;
};

#endif //BJTU_WGPU_RENDERER_CHILDCOMPONENT_H