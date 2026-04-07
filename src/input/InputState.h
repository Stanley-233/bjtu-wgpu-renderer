#ifndef BJTU_WGPU_RENDERER_INPUTSTATE_H
#define BJTU_WGPU_RENDERER_INPUTSTATE_H

#include <unordered_map>

#include <GLFW/glfw3.h>

struct InputStateUpdate {
    bool isPressLike     = false;
    bool isRelease       = false;
    bool keyStateChanged = false;
    bool modifierChanged = false;
};

struct InputState {
    std::unordered_map<int, bool> keyPressed{};
    bool                          shiftPressed = false;
    bool                          altPressed   = false;
    bool                          ctrlPressed  = false;

    [[nodiscard]] bool IsPressed(const int key) const {
        const auto it = keyPressed.find(key);
        return it != keyPressed.end() && it->second;
    }

    InputStateUpdate ApplyKeyEvent(const int key, const int action, const int mods) {
        InputStateUpdate update{};
        update.isPressLike = action == GLFW_PRESS || action == GLFW_REPEAT;
        update.isRelease   = action == GLFW_RELEASE;
        if (!(update.isPressLike || update.isRelease)) {
            return update;
        }

        const bool nextPressed = update.isPressLike;
        const bool prevPressed = IsPressed(key);
        if (prevPressed != nextPressed) {
            keyPressed[key]         = nextPressed;
            update.keyStateChanged = true;
        }

        const bool nextShift = (mods & GLFW_MOD_SHIFT) != 0;
        const bool nextAlt   = (mods & GLFW_MOD_ALT) != 0;
        const bool nextCtrl  = (mods & GLFW_MOD_CONTROL) != 0;
        if (nextShift != shiftPressed || nextAlt != altPressed || nextCtrl != ctrlPressed) {
            update.modifierChanged = true;
            shiftPressed           = nextShift;
            altPressed             = nextAlt;
            ctrlPressed            = nextCtrl;
        }

        return update;
    }
};

#endif // BJTU_WGPU_RENDERER_INPUTSTATE_H
