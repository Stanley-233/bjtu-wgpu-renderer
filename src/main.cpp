#define WEBGPU_CPP_IMPLEMENTATION

#include <toml.hpp>
#include <magic_enum.hpp>
#include "app/Application.h"
#include "app/ConfigPaths.h"

int main() {
    toml::table config;
    const std::string configPath = ConfigPaths::Resolve("config.toml").string();
    try {
        config = toml::parse_file(configPath);
    } catch (const toml::parse_error& err) {
        std::cerr << "Failed to parse config at '" << configPath << "': " << err.description() << std::endl;
        return 1;
    } catch (const std::exception& err) {
        std::cerr << "Failed to load config at '" << configPath << "': " << err.what() << std::endl;
        return 1;
    }

    const int         width         = config["window"]["width"].value<int>().value();
    const int         height        = config["window"]["height"].value<int>().value();
    const std::string surfaceFormat = config["render"]["surface_format"].value_or("BGRA8Unorm");
    const double      maxDevicePixelRatio = config["render"]["max_device_pixel_ratio"].value_or(2.0);
    const bool        applicationDebugEnabled = config["Debug"]["application"].value_or(false);
    const bool        inputDebugEnabled       = config["Debug"]["input"].value_or(false);

    Application app;

    if (surfaceFormat == "Auto") {
        app.SetSurfaceFormat(wgpu::TextureFormat::Undefined);
    } else if (surfaceFormat == "BGRA8Unorm") {
        app.SetSurfaceFormat(wgpu::TextureFormat::BGRA8Unorm);
    } else if (surfaceFormat == "RGBA8Unorm") {
        app.SetSurfaceFormat(wgpu::TextureFormat::RGBA8Unorm);
    } else {
        std::cerr << "Unsupported surface format: " << surfaceFormat << std::endl;
        std::cerr << "Supported values: Auto, BGRA8Unorm, RGBA8Unorm" << std::endl;
        return 1;
    }

    const auto success = app
        .SetWindowSize(width, height)
        .SetMaxDevicePixelRatio(maxDevicePixelRatio)
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
