#ifndef BJTU_WGPU_RENDERER_INPUTEVENTBUS_H
#define BJTU_WGPU_RENDERER_INPUTEVENTBUS_H

#include <signal/dispatcher.hpp>

class InputEventBus {
public:
    [[nodiscard]] entt::dispatcher& Dispatcher() {
        return m_dispatcher;
    }

    [[nodiscard]] const entt::dispatcher& Dispatcher() const {
        return m_dispatcher;
    }

private:
    entt::dispatcher m_dispatcher{};
};

#endif // BJTU_WGPU_RENDERER_INPUTEVENTBUS_H
