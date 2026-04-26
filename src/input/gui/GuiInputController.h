#ifndef BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
#define BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H

#include <string>

class InputEventBus;

class GuiInputController {
public:
    void SetEventBus(InputEventBus* eventBus);

    void BuildUi(const char* activeSceneName);

private:
    InputEventBus* m_eventBus = nullptr;
    int            m_buttonClickCount = 0;
    bool           m_checkboxEnabled = true;
    std::string    m_sceneNameCache{};
};

#endif // BJTU_WGPU_RENDERER_GUIINPUTCONTROLLER_H
