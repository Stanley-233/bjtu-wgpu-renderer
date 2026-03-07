#define WEBGPU_CPP_IMPLEMENTATION

#include "application.h"

int main() {
    Application app;

    if (!app.SetWindowSize(640, 480).Initialize()) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    auto callback = [](void* arg) {
        auto *p_app = reinterpret_cast<Application*>(arg);
        p_app->MainLoop();
    };
    emscripten_set_main_loop_arg(callback, &app, 0, true);
#else
    while (app.IsRunning()) {
        app.MainLoop();
    }
#endif

    app.Terminate();

    return 0;
}
