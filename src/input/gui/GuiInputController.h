#ifndef BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
#define BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H

#include <string>

#include "asset/types/MaterialAsset.h"
#include "render/scene/RenderUniformData.h"

class InputEventBus;
class IScene;
class LegacyGuiRenderer;

class GuiInputController {
public:
    void SetEventBus(InputEventBus* eventBus);

    void ConfigureDebugPanel(
        LegacyGuiRenderer& guiRenderer,
        IScene* activeScene,
        bool* ssaoEnabled,
        SsrSettings* ssrSettings,
        ToneMapSettings* toneMapSettings,
        DofSettings* dofSettings,
        EMaterialShadingModel* litShadingModel,
        EPbrDebugView* pbrDebugView,
        bool* playgroundMagentaPointLightEnabled,
        bool* playgroundBluePointLightEnabled,
        bool* roomSpotLightEnabled);

private:
    void DrawUi(
        IScene* activeScene,
        bool* ssaoEnabled,
        SsrSettings* ssrSettings,
        ToneMapSettings* toneMapSettings,
        DofSettings* dofSettings,
        EMaterialShadingModel* litShadingModel,
        EPbrDebugView* pbrDebugView,
        bool* playgroundMagentaPointLightEnabled,
        bool* playgroundBluePointLightEnabled,
        bool* roomSpotLightEnabled);

    InputEventBus* m_eventBus = nullptr;
    std::string    m_sceneNameCache{};
};

#endif // BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
