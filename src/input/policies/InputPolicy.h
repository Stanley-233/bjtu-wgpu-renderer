#ifndef BJTU_WGPU_RENDERER_INPUTPOLICY_H
#define BJTU_WGPU_RENDERER_INPUTPOLICY_H

#include <signal/dispatcher.hpp>

#include "input/InputState.h"

class InputPolicy {
public:
    virtual ~InputPolicy() = default;

    virtual bool Process(
        const InputState& state,
        const InputStateUpdate& update,
        int key,
        int action,
        entt::dispatcher& dispatcher,
        bool& handledPress) = 0;

    [[nodiscard]] virtual bool HandlesKey(int key) const = 0;
};

#endif // BJTU_WGPU_RENDERER_INPUTPOLICY_H
