//
// Created by Stanley on 2026/5/14.
//

#ifndef BJTU_WGPU_RENDERER_PARENTCOMPONENT_H
#define BJTU_WGPU_RENDERER_PARENTCOMPONENT_H

#include <entity/entity.hpp>

struct ParentComponent {
    entt::entity parent = entt::null;
};

#endif //BJTU_WGPU_RENDERER_PARENTCOMPONENT_H