#define WEBGPU_CPP_IMPLEMENTATION

#include <toml.hpp>
#include "app/Application.h"

int main() {
    auto config = toml::parse_file(RESOURCE_DIR "/config.toml");

    const int         width         = config["window"]["width"].value<int>().value();
    const int         height        = config["window"]["height"].value<int>().value();
    const std::string surfaceFormat = config["render"]["surface_format"].value_or("BGRA8Unorm");
    const bool        applicationDebugEnabled = config["Debug"]["application"].value_or(false);
    const bool        inputDebugEnabled       = config["Debug"]["input"].value_or(false);

    Application app;

    if (surfaceFormat == "BGRA8Unorm") {
        app.SetSurfaceFormat(wgpu::TextureFormat::BGRA8Unorm);
    } else {
        std::cerr << "Unsupported surface format: " << surfaceFormat << std::endl;
        return 1;
    }

    const auto success = app
        .SetWindowSize(width, height)
        .SetApplicationDebugEnabled(applicationDebugEnabled)
        .SetInputDebugEnabled(inputDebugEnabled)
        .Initialize();

    if (!success) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }

#ifdef __EMSCRIPTEN__
    auto callback = [](void* arg) {
        auto* pApp = reinterpret_cast<Application*>(arg);
        pApp->MainLoop();
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
