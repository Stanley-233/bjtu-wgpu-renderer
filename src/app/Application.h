#ifndef BJTU_WGPU_RENDERER_APPLICATION_H
#define BJTU_WGPU_RENDERER_APPLICATION_H

#include <GLFW/glfw3.h>
#include <optional>

#include "app/WindowContext.h"
#include "asset/types/MaterialAsset.h"
#include "input/InputEventBus.h"
#include "input/InputManager.h"
#include "input/gui/GuiInputController.h"
#include "render/RenderContext.h"
#include "render/legacy/LegacyGuiRenderer.h"
#include "scene/SceneManager.h"

class Application {
public:
    Application& SetWindowSize(int width, int height);

    Application& SetSurfaceFormat(wgpu::TextureFormat format);

    Application& SetMaxDevicePixelRatio(double maxDevicePixelRatio);

    Application& SetApplicationDebugEnabled(bool enabled);

    Application& SetInputDebugEnabled(bool enabled);

    bool Initialize();

    void Terminate();

    void MainLoop();

    [[nodiscard]] bool IsRunning() const;

private:
    static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    static void GLFWCursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    void Tick(float deltaTime);

    void HandleKey(int key, int action, int mods);

    void HandleMouseButton(int button, int action, int mods);

    void HandleCursorPos(double xpos, double ypos);

    void SwitchScene(ESceneType type);

    void ApplyActiveSceneRenderSettings() const;

    void OnSceneSwitchRequest(const SceneSwitchRequest& request);

    void OnToggleCameraModeRequest(const ToggleCameraModeRequest& request);

    int                       m_windowWidth         = 640;
    int                       m_windowHeight        = 480;
    double                    m_maxDevicePixelRatio = 2.0;
    wgpu::TextureFormat       m_surfaceFormat       = wgpu::TextureFormat::Undefined;
    WindowContext             m_windowContext;
    RenderContext             m_renderContext;
    LegacyGuiRenderer         m_guiRenderer;
    GuiInputController        m_guiInputController;
    SceneManager              m_sceneManager;
    InputEventBus             m_inputEventBus;
    InputManager              m_inputManager;
    double                    m_lastFrameTime           = 0.0;
    bool                      m_applicationDebugEnabled = false;
    bool                      m_inputDebugEnabled       = false;
    bool                      m_ssaoEnabled             = true;
    SsrSettings               m_ssrSettings{};
    ToneMapSettings           m_toneMapSettings{};
    DofSettings               m_dofSettings{};
    EMaterialShadingModel     m_litShadingModel         = EMaterialShadingModel::Lambert;
    EPbrDebugView             m_pbrDebugView            = EPbrDebugView::Off;
    std::optional<ESceneType> m_pendingSceneSwitch;
    bool                      m_commandHandlersConnected = false;
};

#endif // BJTU_WGPU_RENDERER_APPLICATION_H
