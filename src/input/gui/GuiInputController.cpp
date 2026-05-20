#include "GuiInputController.h"

#include <imgui.h>

#include "scene/IScene.h"
#include "input/InputEventBus.h"

static const char* PbrDebugViewLabel(const EPbrDebugView debugView) {
    switch (debugView) {
        case EPbrDebugView::Off:
            return "Off";
        case EPbrDebugView::GeometricNormal:
            return "Geometric Normal";
        case EPbrDebugView::NormalMapWorld:
            return "Normal Map (WS)";
        case EPbrDebugView::NormalDelta:
            return "Normal Delta";
    }
    return "Unknown";
}

void GuiInputController::SetEventBus(InputEventBus* eventBus) {
    m_eventBus = eventBus;
}

void GuiInputController::BuildUi(
    const char*            activeSceneName,
    bool*                  ssaoEnabled,
    EMaterialShadingModel* litShadingModel,
    EPbrDebugView*         pbrDebugView) {
    m_sceneNameCache = (activeSceneName == nullptr) ? "Unknown" : activeSceneName;

    ImGui::Begin("Hello, world!");
    ImGui::Text("Active scene: %s", m_sceneNameCache.c_str());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    if (ssaoEnabled != nullptr) {
        ImGui::Checkbox("Enable SSAO", ssaoEnabled);
    }
    if (litShadingModel != nullptr) {
        int shadingModeIndex = *litShadingModel == EMaterialShadingModel::Pbr ? 1 : 0;
        ImGui::Text("Lit Shading");
        ImGui::RadioButton("Lambert", &shadingModeIndex, 0);
        ImGui::SameLine();
        ImGui::RadioButton("PBR", &shadingModeIndex, 1);
        *litShadingModel = shadingModeIndex == 1 ?
                               EMaterialShadingModel::Pbr :
                               EMaterialShadingModel::Lambert;
    }
    if (pbrDebugView != nullptr) {
        ImGui::BeginDisabled(litShadingModel == nullptr || *litShadingModel != EMaterialShadingModel::Pbr);
        if (ImGui::BeginCombo("PBR Normal Debug", PbrDebugViewLabel(*pbrDebugView))) {
            constexpr EPbrDebugView debugViews[] = {
                EPbrDebugView::Off,
                EPbrDebugView::GeometricNormal,
                EPbrDebugView::NormalMapWorld,
                EPbrDebugView::NormalDelta,
            };
            for (const EPbrDebugView debugView : debugViews) {
                const bool isSelected = *pbrDebugView == debugView;
                if (ImGui::Selectable(PbrDebugViewLabel(debugView), isSelected)) {
                    *pbrDebugView = debugView;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
    }

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
    }

    ImGui::End();
}
