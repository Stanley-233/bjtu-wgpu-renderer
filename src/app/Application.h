#ifndef BJTU_WGPU_RENDERER_APPLICATION_H
#define BJTU_WGPU_RENDERER_APPLICATION_H

#include <GLFW/glfw3.h>

#include "../render/RenderContext.h"
#include "../scene/SceneManager.h"

class Application {
public:
    bool enableMainLoopDebug = false;

    Application& SetWindowSize(int width, int height);

    Application& SetSurfaceFormat(wgpu::TextureFormat format);

    bool Initialize();

    void Terminate();

    void MainLoop();

    [[nodiscard]] bool IsRunning() const;

private:
    static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    void Tick(float deltaTime);

    void HandleKey(int key, int action, int mods);

    void SwitchScene(ESceneType type);

    int                 m_windowWidth   = 640;
    int                 m_windowHeight  = 480;
    wgpu::TextureFormat m_surfaceFormat = wgpu::TextureFormat::Undefined;
    RenderContext       m_renderContext;
    SceneManager        m_sceneManager;
    double              m_lastFrameTime = 0.0;
};

#endif // BJTU_WGPU_RENDERER_APPLICATION_H
