#include "Application.h"

#include <iostream>
#include <memory>

#include "scene/LogicScene.h"
#include "scene/legacy/Scene2D.h"
#include "scene/legacy/Scene3DLegacy.h"

Application& Application::SetWindowSize(const int width, const int height) {
    m_windowWidth  = width;
    m_windowHeight = height;
    return *this;
}

Application& Application::SetSurfaceFormat(const wgpu::TextureFormat format) {
    m_surfaceFormat = format;
    return *this;
}

Application& Application::SetMaxDevicePixelRatio(const double maxDevicePixelRatio) {
    m_maxDevicePixelRatio = maxDevicePixelRatio;
    return *this;
}

Application& Application::SetApplicationDebugEnabled(const bool enabled) {
    m_applicationDebugEnabled = enabled;
    return *this;
}

Application& Application::SetInputDebugEnabled(const bool enabled) {
    m_inputDebugEnabled = enabled;
    return *this;
}

bool Application::Initialize() {
    m_windowContext.SetWindowSize(m_windowWidth, m_windowHeight);
    m_windowContext.SetMaxDevicePixelRatio(m_maxDevicePixelRatio);
    m_renderContext.SetSurfaceFormat(m_surfaceFormat);
    m_renderContext.enableFrameDebug = false;

    if (!m_windowContext.Initialize()) {
        return false;
    }

    if (!m_renderContext.Initialize(m_windowContext)) {
        m_windowContext.Terminate();
        return false;
    }

    glfwSetWindowUserPointer(m_windowContext.GetWindow(), this);
    glfwSetKeyCallback(m_windowContext.GetWindow(), GLFWKeyCallback);
    glfwSetMouseButtonCallback(m_windowContext.GetWindow(), GLFWMouseButtonCallback);
    glfwSetCursorPosCallback(m_windowContext.GetWindow(), GLFWCursorPosCallback);
    if (!m_guiRenderer.Initialize(
        m_windowContext.GetWindow(),
        m_renderContext.GetDevice(),
        m_renderContext.GetSurfaceFormat())) {
        m_renderContext.Shutdown();
        m_windowContext.Terminate();
        return false;
    }

    m_guiInputController.SetEventBus(&m_inputEventBus);
    m_inputManager.SetEventBus(&m_inputEventBus);
    m_inputManager.SetDebugEnabled(m_inputDebugEnabled);

    m_sceneManager.RegisterScene(ESceneType::Scene2D, std::make_unique<Scene2D>());
    m_sceneManager.RegisterScene(ESceneType::Scene3DLegacy, std::make_unique<Scene3DLegacy>());
    m_sceneManager.RegisterScene(ESceneType::LogicScene, std::make_unique<LogicScene>());
    m_sceneManager.InitializeAll(m_renderContext);
    m_sceneManager.SetInputEventBus(m_inputEventBus);
    m_sceneManager.SetActiveScene(ESceneType::LogicScene);

    m_inputEventBus.Dispatcher().sink<SceneSwitchRequest>().connect<&Application::OnSceneSwitchRequest>(*this);
    m_inputEventBus.Dispatcher().sink<ToggleCameraModeRequest>().connect<&Application::OnToggleCameraModeRequest>(*this);
    m_commandHandlersConnected = true;

    m_lastFrameTime = glfwGetTime();
    return true;
}

void Application::Terminate() {
    if (m_commandHandlersConnected) {
        m_inputEventBus.Dispatcher().sink<SceneSwitchRequest>().disconnect<&Application::OnSceneSwitchRequest>(*this);
        m_inputEventBus.Dispatcher().sink<ToggleCameraModeRequest>().disconnect<&Application::OnToggleCameraModeRequest>(*this);
        m_commandHandlersConnected = false;
    }
    m_inputManager.SetEventBus(nullptr);
    m_guiInputController.SetEventBus(nullptr);
    m_guiRenderer.Shutdown();
    m_renderContext.Shutdown();
    m_windowContext.Terminate();
}

bool Application::IsRunning() const {
    return m_windowContext.IsRunning();
}

void Application::MainLoop() {
    m_windowContext.PollEvents();

    const double currentTime = glfwGetTime();
    const float  deltaTime   = static_cast<float>(currentTime - m_lastFrameTime);
    m_lastFrameTime          = currentTime;

    Tick(deltaTime);
}

void Application::GLFWKeyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
    (void)scancode;
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr) {
        return;
    }
    application->HandleKey(key, action, mods);
}

void Application::GLFWMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods) {
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr) {
        return;
    }
    application->HandleMouseButton(button, action, mods);
}

void Application::GLFWCursorPosCallback(GLFWwindow* window, const double xpos, const double ypos) {
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr) {
        return;
    }
    application->HandleCursorPos(xpos, ypos);
}

void Application::Tick(float deltaTime) {
    m_sceneManager.UpdateActive(deltaTime);

    int drawableWidth = 0;
    int drawableHeight = 0;
    m_windowContext.GetDrawableSize(drawableWidth, drawableHeight);
    m_guiRenderer.BeginFrame(drawableWidth, drawableHeight);
    m_guiInputController.BuildUi(m_sceneManager.ActiveScene().Name());
    m_guiRenderer.EndFrame();

    m_sceneManager.RenderActive(m_renderContext, m_guiRenderer);
}

void Application::HandleKey(int key, int action, int mods) {
    if (m_guiRenderer.WantCaptureKeyboard()) {
        return;
    }
    m_inputManager.EmitKeyEvent(key, action, mods);
}

void Application::HandleMouseButton(const int button, const int action, const int mods) {
    // TODO：后续在这里把 RMB 按下/释放接入 UE 风格漫游视角控制启停链路。
    m_inputManager.EmitMouseButtonEvent(button, action, mods);
}

void Application::HandleCursorPos(const double xpos, const double ypos) {
    // TODO：后续在 RMB 视角控制模式生效后，把鼠标移动转成相机视角输入。
    m_inputManager.EmitCursorPosEvent(xpos, ypos);
}

void Application::SwitchScene(ESceneType type) {
    m_sceneManager.SetActiveScene(type);
}

void Application::OnSceneSwitchRequest(const SceneSwitchRequest& request) {
    if (m_applicationDebugEnabled) {
        std::cout << "[Application] Scene switch request" << std::endl;
    }
    SwitchScene(request.type);
    if (m_applicationDebugEnabled) {
        std::cout << "[Application] Active scene: " << m_sceneManager.ActiveScene().Name() << std::endl;
    }
}

void Application::OnToggleCameraModeRequest(const ToggleCameraModeRequest& request) {
    (void)request;
    if (m_applicationDebugEnabled) {
        std::cout << "[Application] Toggle camera mode request" << std::endl;
    }
}
