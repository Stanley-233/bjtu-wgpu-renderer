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

static const char* ToneMapExposureModeLabel(const EToneMapExposureMode mode) {
    switch (mode) {
        case EToneMapExposureMode::ManualEv:
            return "Manual EV";
        case EToneMapExposureMode::AutoExposure:
            return "Auto Exposure";
    }
    return "Unknown";
}

static const char* DofDebugModeLabel(const EDoFDebugMode mode) {
    switch (mode) {
        case EDoFDebugMode::Off:
            return "Off";
        case EDoFDebugMode::FocusPlaneTint:
            return "Focus Plane Tint";
    }
    return "Unknown";
}

void GuiInputController::SetEventBus(InputEventBus* eventBus) {
    m_eventBus = eventBus;
}

void GuiInputController::BuildUi(
    const char*            activeSceneName,
    bool*                  ssaoEnabled,
    SsrSettings*           ssrSettings,
    ToneMapSettings*       toneMapSettings,
    DofSettings*           dofSettings,
    EMaterialShadingModel* litShadingModel,
    EPbrDebugView*         pbrDebugView,
    bool*                  playgroundMagentaPointLightEnabled,
    bool*                  playgroundBluePointLightEnabled,
    bool*                  roomSpotLightEnabled) {
    m_sceneNameCache = (activeSceneName == nullptr) ? "Unknown" : activeSceneName;

    if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Active scene: %s", m_sceneNameCache.c_str());
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

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

        if (m_sceneNameCache == "ScenePlayground") {
            if (playgroundMagentaPointLightEnabled != nullptr) {
                ImGui::Checkbox("Magenta Point Light", playgroundMagentaPointLightEnabled);
            }
            if (playgroundBluePointLightEnabled != nullptr) {
                ImGui::Checkbox("Blue Point Light", playgroundBluePointLightEnabled);
            }
        } else if (m_sceneNameCache == "SceneRoom") {
            if (roomSpotLightEnabled != nullptr) {
                ImGui::Checkbox("Room Spot Light", roomSpotLightEnabled);
            }
        }
    }

    if (ImGui::CollapsingHeader("Shading", ImGuiTreeNodeFlags_DefaultOpen)) {
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
    }

    if (ImGui::CollapsingHeader("Postprocessing", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ssaoEnabled != nullptr) {
                ImGui::Checkbox("Enable SSAO", ssaoEnabled);
            }
        }
        if (ImGui::CollapsingHeader("SSR", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ssrSettings != nullptr) {
                ImGui::Checkbox("Enable SSR", &ssrSettings->enabled);
                ImGui::SliderFloat("SSR Strength", &ssrSettings->strength, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("SSR Max Distance", &ssrSettings->maxDistance, 0.1f, 50.0f, "%.2f");
                ImGui::SliderFloat("SSR Thickness", &ssrSettings->thickness, 0.01f, 1.0f, "%.3f");
            }
        }
        if (ImGui::CollapsingHeader("EV", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (toneMapSettings != nullptr) {
                int exposureMode = static_cast<int>(toneMapSettings->exposureMode);
                if (ImGui::BeginCombo("Exposure Mode", ToneMapExposureModeLabel(toneMapSettings->exposureMode))) {
                    constexpr EToneMapExposureMode modes[] = {
                        EToneMapExposureMode::ManualEv,
                        EToneMapExposureMode::AutoExposure,
                    };
                    for (const EToneMapExposureMode mode : modes) {
                        const bool isSelected = toneMapSettings->exposureMode == mode;
                        if (ImGui::Selectable(ToneMapExposureModeLabel(mode), isSelected)) {
                            exposureMode = static_cast<int>(mode);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                toneMapSettings->exposureMode = static_cast<EToneMapExposureMode>(exposureMode);

                ImGui::BeginDisabled(toneMapSettings->exposureMode != EToneMapExposureMode::ManualEv);
                ImGui::SliderFloat("Exposure EV", &toneMapSettings->exposureEv, -8.0f, 8.0f, "%.2f EV");
                ImGui::EndDisabled();
            }
        }
        if (ImGui::CollapsingHeader("DoF", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (dofSettings != nullptr) {
                ImGui::Checkbox("Enable DoF", &dofSettings->enabled);
                ImGui::SliderFloat("Focus Distance", &dofSettings->focusDistance, 0.1f, 50.0f, "%.2f");
                ImGui::SliderFloat("Focus Range", &dofSettings->focusRange, 0.01f, 10.0f, "%.2f");
                ImGui::SliderFloat("Max Blur Radius", &dofSettings->maxBlurRadiusPixels, 0.0f, 32.0f, "%.1f px");

                int debugMode = static_cast<int>(dofSettings->debugMode);
                if (ImGui::BeginCombo("DoF Debug", DofDebugModeLabel(dofSettings->debugMode))) {
                    constexpr EDoFDebugMode modes[] = {
                        EDoFDebugMode::Off,
                        EDoFDebugMode::FocusPlaneTint,
                    };
                    for (const EDoFDebugMode mode : modes) {
                        const bool isSelected = dofSettings->debugMode == mode;
                        if (ImGui::Selectable(DofDebugModeLabel(mode), isSelected)) {
                            debugMode = static_cast<int>(mode);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                dofSettings->debugMode = static_cast<EDoFDebugMode>(debugMode);

                ImGui::BeginDisabled(dofSettings->debugMode != EDoFDebugMode::FocusPlaneTint);
                ImGui::SliderFloat(
                    "Debug Plane Thickness",
                    &dofSettings->debugPlaneHalfThickness,
                    0.005f,
                    1.0f,
                    "%.3f");
                ImGui::EndDisabled();
            }
        }
    }
}
