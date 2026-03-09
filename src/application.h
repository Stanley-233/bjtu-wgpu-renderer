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

    // Set the size of the window
    Application& SetWindowSize(int width, int height);

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

    // Substep of Initialize() that creates the render pipeline
    void InitializePipeline();

    void TestBuffers();

    // We put here all the variables that are shared between init and main loop
    int                                  m_windowWidth   = 640;
    int                                  m_windowHeight  = 480;
    GLFWwindow*                          m_window         = nullptr;
    wgpu::raii::Device                   m_device;
    wgpu::raii::Queue                    m_queue;
    wgpu::raii::Surface                  m_surface;
    std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallbackHandle;
    wgpu::TextureFormat                  m_surfaceFormat = wgpu::TextureFormat::Undefined;
    wgpu::raii::RenderPipeline           m_pipeline;
};


#endif //BJTU_WGPU_RENDERER_APPLICATION_H