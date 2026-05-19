#include "GuiInputController.h"

#include <imgui.h>

#include "scene/IScene.h"
#include "input/InputEventBus.h"

void GuiInputController::SetEventBus(InputEventBus* eventBus) {
    m_eventBus = eventBus;
}

void GuiInputController::BuildUi(const char* activeSceneName) {
    m_sceneNameCache = (activeSceneName == nullptr) ? "Unknown" : activeSceneName;

    ImGui::Begin("Hello, world!");
    ImGui::Text("Active scene: %s", m_sceneNameCache.c_str());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    if (ImGui::Button("Button")) {
        ++m_buttonClickCount;
    }
    ImGui::SameLine();
    ImGui::Text("clicks = %d", m_buttonClickCount);
    ImGui::Checkbox("Sample checkbox", &m_checkboxEnabled);

    if (m_eventBus != nullptr) {
        if (ImGui::Button("Switch Scene2D")) {
            m_eventBus->Dispatcher().trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::Scene2D});
        }
        ImGui::SameLine();
        if (ImGui::Button("Switch Playground")) {
            m_eventBus->Dispatcher().trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::ScenePlayground});
        }
        if (ImGui::Button("Switch Sponza")) {
            m_eventBus->Dispatcher().trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::SceneSponza});
        }
        ImGui::SameLine();
        if (ImGui::Button("Switch Room")) {
            m_eventBus->Dispatcher().trigger<SceneSwitchRequest>(SceneSwitchRequest{ESceneType::SceneRoom});
        }
        if (ImGui::Button("Toggle Camera Mode")) {
            m_eventBus->Dispatcher().trigger<ToggleCameraModeRequest>(ToggleCameraModeRequest{});
        }
    }

    ImGui::End();
}
