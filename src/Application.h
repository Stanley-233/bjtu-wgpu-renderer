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

    Application& SetSurfaceFormat(wgpu::TextureFormat format);

    // Initialize everything and return true if it went all right
    bool Initialize();

    // Uninitialize everything that was initialized
    void Terminate();

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    [[nodiscard]] bool IsRunning() const;

private:
    // Substep of MainLoop() that gets the next surface texture and creates a view for it
    [[nodiscard]] wgpu::raii::TextureView GetNextSurfaceTextureView();

    // Substep of Initialize() that creates the render pipeline
    void InitializePipeline();

    // Substep of Initialize() that tests buffer creation, copying and mapping
    void TestBuffers();

    // Substep of Initialize() that queries the device limits and returns a RequiredLimits struct
    static wgpu::RequiredLimits GetRequiredLimits(wgpu::Adapter adapter);

    // Initialization attributes
    int                                  m_windowWidth  = 640;
    int                                  m_windowHeight = 480;
    GLFWwindow*                          m_window       = nullptr;
    wgpu::raii::Device                   m_device;
    wgpu::raii::Queue                    m_queue;
    wgpu::raii::Surface                  m_surface;
    wgpu::raii::Buffer                   m_uniformBuffer;
    wgpu::raii::PipelineLayout           m_layout;
    wgpu::raii::BindGroupLayout          m_bindGroupLayout;
    wgpu::raii::BindGroup                m_bindGroup;
    std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallbackHandle;
    wgpu::TextureFormat                  m_surfaceFormat = wgpu::TextureFormat::Undefined;
    wgpu::raii::RenderPipeline           m_pipeline;

    // Buffer
    wgpu::raii::Buffer m_pointBuffer;
    wgpu::raii::Buffer m_indexBuffer;
    uint32_t           m_indexCount = 0;

    void InitializeBuffers();

    void InitializeBindGroups();
};


#endif //BJTU_WGPU_RENDERER_APPLICATION_H