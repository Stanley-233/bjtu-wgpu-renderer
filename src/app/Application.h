#ifndef BJTU_WGPU_RENDERER_APPLICATION_H
#define BJTU_WGPU_RENDERER_APPLICATION_H

#include <GLFW/glfw3.h>

#include "../input/InputManager.h"
#include "../render/RenderContext.h"
#include "../scene/SceneManager.h"

class Application {
public:
    Application& SetWindowSize(int width, int height);

    Application& SetSurfaceFormat(wgpu::TextureFormat format);

    Application& SetApplicationDebugEnabled(bool enabled);

    Application& SetInputDebugEnabled(bool enabled);

    bool Initialize();

    void Terminate();

    void MainLoop();

    [[nodiscard]] bool IsRunning() const;

private:
    static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    void Tick(float deltaTime);

    void HandleKey(int key, int action, int mods);

    void SwitchScene(ESceneType type);

    void BindInputForActiveScene();

    void UnbindInputFromActiveScene();

    int                 m_windowWidth   = 640;
    int                 m_windowHeight  = 480;
    wgpu::TextureFormat m_surfaceFormat = wgpu::TextureFormat::Undefined;
    RenderContext       m_renderContext;
    SceneManager        m_sceneManager;
    InputManager        m_inputManager;
    IScene*             m_boundInputScene = nullptr;
    double              m_lastFrameTime = 0.0;
    bool                m_applicationDebugEnabled = false;
    bool                m_inputDebugEnabled       = false;
};

#endif // BJTU_WGPU_RENDERER_APPLICATION_H
