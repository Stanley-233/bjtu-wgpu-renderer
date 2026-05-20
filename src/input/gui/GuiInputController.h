#ifndef BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
#define BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H

#include <string>

#include "asset/types/MaterialAsset.h"

class InputEventBus;

class GuiInputController {
public:
    void SetEventBus(InputEventBus* eventBus);

    void BuildUi(const char* activeSceneName, bool* ssaoEnabled, EMaterialShadingModel* litShadingModel);

private:
    InputEventBus* m_eventBus = nullptr;
    std::string    m_sceneNameCache{};
};

#endif // BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
