#ifndef BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
#define BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H

#include <string>

class InputEventBus;

class GuiInputController {
public:
    void SetEventBus(InputEventBus* eventBus);

    void BuildUi(const char* activeSceneName, bool* ssaoEnabled);

private:
    InputEventBus* m_eventBus = nullptr;
    std::string    m_sceneNameCache{};
};

#endif // BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
