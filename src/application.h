//
// Created by Stanley on 2026/3/6.
//

#ifndef BJTU_WGPU_RENDERER_APPLICATION_H
#define BJTU_WGPU_RENDERER_APPLICATION_H

#include <GLFW/glfw3.h>

#include <webgpu/webgpu.hpp>

class Application {
public:
    // Initialize everything and return true if it went all right
    bool Initialize();

    // Uninitialize everything that was initialized
    void Terminate() const;

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    bool IsRunning() const;

private:
    // We put here all the variables that are shared between init and main loop
    GLFWwindow *window;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
};


#endif //BJTU_WGPU_RENDERER_APPLICATION_H