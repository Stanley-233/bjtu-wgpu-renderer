#include "Application.h"

#include <iostream>
#include <memory>

#include "../scene/Scene2D.h"
#include "../scene/scene3d/Scene3D.h"

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
    m_renderContext.SetWindowSize(m_windowWidth, m_windowHeight);
    m_renderContext.SetSurfaceFormat(m_surfaceFormat);
    m_renderContext.SetMaxDevicePixelRatio(m_maxDevicePixelRatio);
    m_renderContext.enableFrameDebug = false;

    if (!m_renderContext.Initialize()) {
        return false;
    }

    glfwSetWindowUserPointer(m_renderContext.GetWindow(), this);
    glfwSetKeyCallback(m_renderContext.GetWindow(), GLFWKeyCallback);
    if (!m_guiRenderer.Initialize(
        m_renderContext.GetWindow(),
        m_renderContext.GetDevice(),
        m_renderContext.GetSurfaceFormat())) {
        return false;
    }

    m_guiInputController.SetEventBus(&m_inputEventBus);
    m_inputManager.SetEventBus(&m_inputEventBus);
    m_inputManager.SetDebugEnabled(m_inputDebugEnabled);

    m_sceneManager.RegisterScene(ESceneType::Scene2D, std::make_unique<Scene2D>());
    m_sceneManager.RegisterScene(ESceneType::Scene3D, std::make_unique<Scene3D>());
    m_sceneManager.InitializeAll(m_renderContext);
    m_sceneManager.SetInputEventBus(m_inputEventBus);
    m_sceneManager.SetActiveScene(ESceneType::Scene3D);

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
    m_renderContext.Terminate();
}

bool Application::IsRunning() const {
    return m_renderContext.IsRunning();
}

void Application::MainLoop() {
    m_renderContext.PollEvents();

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

void Application::Tick(float deltaTime) {
    m_sceneManager.UpdateActive(deltaTime);

    int drawableWidth = 0;
    int drawableHeight = 0;
    m_renderContext.GetDrawableSize(drawableWidth, drawableHeight);
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
