#include "Application.h"

#include <iostream>
#include <memory>

#include <imgui.h>

#include "scene/LogicScene.h"
#include "scene/ScenePlayground.h"
#include "scene/SceneRoom.h"
#include "scene/SceneRoom.h"
#include "scene/SceneSponza.h"
#include "scene/legacy/Scene2D.h"

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

    m_sceneManager.RegisterScene(ESceneType::Scene2D, [] {
        return std::make_unique<Scene2D>();
    });
    m_sceneManager.RegisterScene(ESceneType::ScenePlayground, [] {
        return std::make_unique<ScenePlayground>();
    });
    m_sceneManager.RegisterScene(ESceneType::SceneSponza, [] {
        return std::make_unique<SceneSponza>();
    });
    m_sceneManager.RegisterScene(ESceneType::SceneRoom, [] {
        return std::make_unique<SceneRoom>();
    });
    m_sceneManager.SetInputEventBus(m_inputEventBus);
    if (!m_sceneManager.SetActiveScene(ESceneType::ScenePlayground, m_renderContext)) {
        Terminate();
        return false;
    }
    ApplyActiveSceneRenderSettings();

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
    m_sceneManager.Shutdown();
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
    m_guiRenderer.SetDebugPanelContentCallback([this]() {
        bool* magentaPointLightEnabled = nullptr;
        bool* bluePointLightEnabled = nullptr;
        bool* roomSpotLightEnabled = nullptr;
        auto* playgroundScene = m_sceneManager.HasActiveScene() ?
                                    dynamic_cast<ScenePlayground*>(&m_sceneManager.ActiveScene()) :
                                    nullptr;
        auto* roomScene = m_sceneManager.HasActiveScene() ?
                              dynamic_cast<SceneRoom*>(&m_sceneManager.ActiveScene()) :
                              nullptr;
        bool  magentaEnabledValue = false;
        bool  blueEnabledValue = false;
        bool  roomSpotLightEnabledValue = false;
        if (playgroundScene != nullptr) {
            magentaEnabledValue = playgroundScene->IsMagentaPointLightEnabled();
            blueEnabledValue = playgroundScene->IsBluePointLightEnabled();
            magentaPointLightEnabled = &magentaEnabledValue;
            bluePointLightEnabled = &blueEnabledValue;
        }
        if (roomScene != nullptr) {
            roomSpotLightEnabledValue = roomScene->IsSpotLightEnabled();
            roomSpotLightEnabled = &roomSpotLightEnabledValue;
        }

        m_guiInputController.BuildUi(
            m_sceneManager.HasActiveScene() ? m_sceneManager.ActiveScene().Name() : nullptr,
            &m_ssaoEnabled,
            &m_ssrSettings,
            &m_toneMapSettings,
            &m_dofSettings,
            &m_litShadingModel,
            &m_pbrDebugView,
            magentaPointLightEnabled,
            bluePointLightEnabled,
            roomSpotLightEnabled);

        if (playgroundScene != nullptr) {
            playgroundScene->SetMagentaPointLightEnabled(magentaEnabledValue);
            playgroundScene->SetBluePointLightEnabled(blueEnabledValue);
        }
        if (roomScene != nullptr) {
            roomScene->SetSpotLightEnabled(roomSpotLightEnabledValue);
        }
    });

    if (m_sceneManager.HasActiveScene()) {
        if (auto* logicScene = dynamic_cast<LogicScene*>(&m_sceneManager.ActiveScene())) {
            m_guiRenderer.SetDirectionalLightCallbacks(
                [logicScene]() { return logicScene->GetDirectionalLightData(); },
                [logicScene](const DirectionalLightGuiData& data) { logicScene->SetDirectionalLightData(data); }
            );
            m_guiRenderer.SetCameraInfoCallback(
                [logicScene]() { return logicScene->GetCameraData(); }
            );
        } else {
            m_guiRenderer.SetDirectionalLightCallbacks({}, {});
            m_guiRenderer.SetCameraInfoCallback({});
        }
    } else {
        m_guiRenderer.SetDirectionalLightCallbacks({}, {});
        m_guiRenderer.SetCameraInfoCallback({});
    }

    m_guiRenderer.DrawDebugPanel();

    m_guiRenderer.EndFrame();

    if (m_pendingSceneSwitch.has_value()) {
        SwitchScene(*m_pendingSceneSwitch);
        m_pendingSceneSwitch.reset();
    }

    ApplyActiveSceneRenderSettings();

    m_sceneManager.RenderActive(m_renderContext, m_guiRenderer);
}

void Application::HandleKey(int key, int action, int mods) {
    if (m_guiRenderer.WantCaptureKeyboard()) {
        return;
    }
    m_inputManager.EmitKeyEvent(key, action, mods);
}

void Application::HandleMouseButton(const int button, const int action, const int mods) {
    (void)mods;
    m_inputManager.EmitMouseButtonEvent(button, action, mods);
}

void Application::HandleCursorPos(const double xpos, const double ypos) {
    m_inputManager.EmitCursorPosEvent(xpos, ypos);
}

void Application::SwitchScene(ESceneType type) {
    if (!m_sceneManager.SetActiveScene(type, m_renderContext) && m_applicationDebugEnabled) {
        std::cout << "[Application] Scene switch failed; keeping current scene." << std::endl;
        return;
    }

    if (type == ESceneType::SceneRoom) {
        m_toneMapSettings.exposureMode = EToneMapExposureMode::ManualEv;
        m_toneMapSettings.exposureEv = 1.25f;
    }

    ApplyActiveSceneRenderSettings();
}

void Application::ApplyActiveSceneRenderSettings() const {
    if (!m_sceneManager.HasActiveScene()) {
        return;
    }

    if (auto* logicScene = dynamic_cast<LogicScene*>(&m_sceneManager.ActiveScene())) {
        logicScene->SetSsaoEnabled(m_ssaoEnabled);
        logicScene->SetSsrSettings(m_ssrSettings);
        logicScene->SetToneMapSettings(m_toneMapSettings);
        logicScene->SetDofSettings(m_dofSettings);
        logicScene->SetLitShadingModelOverride(m_litShadingModel);
        logicScene->SetPbrDebugView(m_pbrDebugView);
    }
}

void Application::OnSceneSwitchRequest(const SceneSwitchRequest& request) {
    if (m_applicationDebugEnabled) {
        std::cout << "[Application] Scene switch request" << std::endl;
    }
    m_pendingSceneSwitch = request.type;
    if (m_applicationDebugEnabled && m_sceneManager.HasActiveScene()) {
        std::cout << "[Application] Active scene: " << m_sceneManager.ActiveScene().Name() << std::endl;
    }
}

void Application::OnToggleCameraModeRequest(const ToggleCameraModeRequest& request) {
    (void)request;
    if (m_applicationDebugEnabled) {
        std::cout << "[Application] Toggle camera mode request" << std::endl;
    }
}
