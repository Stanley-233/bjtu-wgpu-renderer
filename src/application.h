//
// Created by Stanley on 2026/3/6.
//

#ifndef BJTU_WGPU_RENDERER_APPLICATION_H
#define BJTU_WGPU_RENDERER_APPLICATION_H

#include <GLFW/glfw3.h>

#include "webgpu-raii.hpp"

class Application {
public:
    // Whether to print debug messages about the rendering process
    bool enableMainLoopDebug = false;

    // Initialize everything and return true if it went all right
    bool Initialize();

    // Uninitialize everything that was initialized
    void Terminate();

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    [[nodiscard]] bool IsRunning() const;

private:
    [[nodiscard]] wgpu::raii::TextureView GetNextSurfaceTextureView();

private:
    // We put here all the variables that are shared between init and main loop
    GLFWwindow *window = nullptr;
    wgpu::raii::Device device;
    wgpu::raii::Queue queue;
    wgpu::raii::Surface surface;
    std::unique_ptr<wgpu::ErrorCallback> uncapturedErrorCallbackHandle;
};


#endif //BJTU_WGPU_RENDERER_APPLICATION_H
