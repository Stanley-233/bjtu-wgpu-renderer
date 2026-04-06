#include "Application.h"

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
    m_renderContext.enableFrameDebug = false;

    if (!m_renderContext.Initialize()) {
        return false;
    }

    glfwSetWindowUserPointer(m_renderContext.GetWindow(), this);
    glfwSetKeyCallback(m_renderContext.GetWindow(), GLFWKeyCallback);

    m_sceneManager.RegisterScene(ESceneType::Scene2D, std::make_unique<Scene2D>());
    m_sceneManager.RegisterScene(ESceneType::Scene3D, std::make_unique<Scene3D>());
    m_sceneManager.InitializeAll(m_renderContext);
    m_sceneManager.SetActiveScene(ESceneType::Scene3D);
    m_inputManager.SetDebugEnabled(m_inputDebugEnabled);
    BindInputForActiveScene();

    m_lastFrameTime = glfwGetTime();
    return true;
}

void Application::Terminate() {
    UnbindInputFromActiveScene();
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
    m_sceneManager.RenderActive(m_renderContext);
}

void Application::HandleKey(int key, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_1) {
        if (m_applicationDebugEnabled) {
            std::cout << "[Application] Key 1" << std::endl;
        }
        SwitchScene(ESceneType::Scene2D);
        if (m_applicationDebugEnabled) {
            std::cout << "[Application] Switch to Scene2D" << std::endl;
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_2) {
        if (m_applicationDebugEnabled) {
            std::cout << "[Application] Key 2" << std::endl;
        }
        SwitchScene(ESceneType::Scene3D);
        if (m_applicationDebugEnabled) {
            std::cout << "[Application] Switch to Scene3D" << std::endl;
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_C) {
        if (auto* scene3D = dynamic_cast<Scene3D*>(&m_sceneManager.ActiveScene()); scene3D != nullptr) {
            scene3D->ToggleCameraMode();
        }
    }

    m_inputManager.EmitKeyEvent(key, action, mods);
}

void Application::SwitchScene(ESceneType type) {
    UnbindInputFromActiveScene();
    m_sceneManager.SetActiveScene(type);
    BindInputForActiveScene();
}

void Application::BindInputForActiveScene() {
    m_boundInputScene = &m_sceneManager.ActiveScene();
    m_inputManager.SubscribeTransformActions(*m_boundInputScene);
}

void Application::UnbindInputFromActiveScene() {
    if (m_boundInputScene == nullptr) {
        return;
    }
    m_inputManager.UnsubscribeTransformActions(*m_boundInputScene);
    m_boundInputScene = nullptr;
}
