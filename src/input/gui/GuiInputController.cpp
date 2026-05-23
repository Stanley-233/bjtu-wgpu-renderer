#include "GuiInputController.h"

#include <cmath>
#include <optional>

#include <imgui.h>

#include "input/InputEventBus.h"
#include "render/legacy/LegacyGuiRenderer.h"
#include "scene/IScene.h"
#include "scene/LogicScene.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;

const char* PbrDebugViewLabel(const EPbrDebugView debugView) {
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

const char* ToneMapExposureModeLabel(const EToneMapExposureMode mode) {
    switch (mode) {
        case EToneMapExposureMode::ManualEv:
            return "Manual EV";
        case EToneMapExposureMode::AutoExposure:
            return "Auto Exposure";
    }
    return "Unknown";
}

const char* DofDebugModeLabel(const EDoFDebugMode mode) {
    switch (mode) {
        case EDoFDebugMode::Off:
            return "Off";
        case EDoFDebugMode::FocusPlaneTint:
            return "Focus Plane Tint";
    }
    return "Unknown";
}

float RadiansToDegrees(const float radians) {
    return radians * (180.0f / kPi);
}

float DegreesToRadians(const float degrees) {
    return degrees * (kPi / 180.0f);
}
}

void GuiInputController::SetEventBus(InputEventBus* eventBus) {
    m_eventBus = eventBus;
}

void GuiInputController::ConfigureDebugPanel(
    LegacyGuiRenderer&     guiRenderer,
    IScene*                activeScene,
    bool*                  ssaoEnabled,
    SsrSettings*           ssrSettings,
    ToneMapSettings*       toneMapSettings,
    DofSettings*           dofSettings,
    EMaterialShadingModel* litShadingModel,
    EPbrDebugView*         pbrDebugView,
    bool*                  playgroundMagentaPointLightEnabled,
    bool*                  playgroundBluePointLightEnabled,
    bool*                  roomSpotLightEnabled) {
    guiRenderer.SetDebugPanelContentCallback([=, this]() {
        DrawUi(
            activeScene,
            ssaoEnabled,
            ssrSettings,
            toneMapSettings,
            dofSettings,
            litShadingModel,
            pbrDebugView,
            playgroundMagentaPointLightEnabled,
            playgroundBluePointLightEnabled,
            roomSpotLightEnabled);
    });
}

void GuiInputController::DrawUi(
    IScene*                activeScene,
    bool*                  ssaoEnabled,
    SsrSettings*           ssrSettings,
    ToneMapSettings*       toneMapSettings,
    DofSettings*           dofSettings,
    EMaterialShadingModel* litShadingModel,
    EPbrDebugView*         pbrDebugView,
    bool*                  playgroundMagentaPointLightEnabled,
    bool*                  playgroundBluePointLightEnabled,
    bool*                  roomSpotLightEnabled) {
    m_sceneNameCache = activeScene == nullptr ? "Unknown" : activeScene->Name();
    auto* logicScene = dynamic_cast<LogicScene*>(activeScene);

    if (ImGui::BeginTabBar("debug_panel_sections")) {
        if (ImGui::BeginTabItem("General")) {
            ImGui::SeparatorText("Application");
            ImGui::Text("Active scene: %s", m_sceneNameCache.c_str());
            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
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

            ImGui::SeparatorText("Camera");
            if (logicScene != nullptr) {
                const CameraGuiData camera = logicScene->GetCameraData();
                ImGui::Text("Position:");
                ImGui::Text("  X: %.2f", camera.position.x);
                ImGui::SameLine();
                ImGui::Text("Y: %.2f", camera.position.y);
                ImGui::SameLine();
                ImGui::Text("Z: %.2f", camera.position.z);

                ImGui::Text("Target:");
                ImGui::Text("  X: %.2f", camera.target.x);
                ImGui::SameLine();
                ImGui::Text("Y: %.2f", camera.target.y);
                ImGui::SameLine();
                ImGui::Text("Z: %.2f", camera.target.z);

                const std::optional<float> fovDegrees = logicScene->GetPerspectiveCameraFovDegrees();
                if (fovDegrees.has_value()) {
                    float fovValue = *fovDegrees;
                    if (ImGui::SliderFloat("FOV", &fovValue, 15.0f, 120.0f, "%.1f deg")) {
                        logicScene->SetPerspectiveCameraFovDegrees(fovValue);
                    }
                } else {
                    ImGui::TextColored(
                        ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                        "FOV control unavailable for current camera");
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No camera data available");
            }

            ImGui::SeparatorText("Lighting");
            if (logicScene != nullptr) {
                DirectionalLightGuiData light = logicScene->GetDirectionalLightData();
                const float dirLength = std::sqrt(
                    light.direction.x * light.direction.x
                    + light.direction.y * light.direction.y
                    + light.direction.z * light.direction.z);
                const glm::vec3 normalizedDirection = dirLength > 0.001f
                                                          ? light.direction / dirLength
                                                          : glm::vec3{0.0f, -1.0f, 0.0f};
                float yawDegrees = RadiansToDegrees(std::atan2(normalizedDirection.x, -normalizedDirection.z));
                float pitchDegrees = RadiansToDegrees(std::asin(normalizedDirection.y));

                bool directionChanged = false;
                directionChanged |= ImGui::SliderFloat("Yaw", &yawDegrees, -180.0f, 180.0f, "%.1f deg");
                directionChanged |= ImGui::SliderFloat("Pitch", &pitchDegrees, -89.0f, 89.0f, "%.1f deg");
                if (directionChanged) {
                    const float yawRadians = DegreesToRadians(yawDegrees);
                    const float pitchRadians = DegreesToRadians(pitchDegrees);
                    const float cosPitch = std::cos(pitchRadians);
                    light.direction.x = std::sin(yawRadians) * cosPitch;
                    light.direction.y = std::sin(pitchRadians);
                    light.direction.z = -std::cos(yawRadians) * cosPitch;
                    logicScene->SetDirectionalLightData(light);
                }

                ImGui::Text("Direction Vector:");
                ImGui::Text("  X: %.3f", normalizedDirection.x);
                ImGui::SameLine();
                ImGui::Text("Y: %.3f", normalizedDirection.y);
                ImGui::SameLine();
                ImGui::Text("Z: %.3f", normalizedDirection.z);

                if (ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f, "%.2f")) {
                    logicScene->SetDirectionalLightData(light);
                }

                float color[3] = {light.color.r, light.color.g, light.color.b};
                if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB)) {
                    light.color.r = color[0];
                    light.color.g = color[1];
                    light.color.b = color[2];
                    logicScene->SetDirectionalLightData(light);
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No directional light data available");
            }

            ImGui::SeparatorText("Shading");
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

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Postprocessing")) {
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
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
