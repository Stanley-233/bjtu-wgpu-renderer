#ifndef BJTU_WGPU_RENDERER_WINDOWCONTEXT_H
#define BJTU_WGPU_RENDERER_WINDOWCONTEXT_H

#include <GLFW/glfw3.h>

class WindowContext {
public:
    WindowContext& SetWindowSize(int width, int height);

    WindowContext& SetMaxDevicePixelRatio(double maxDevicePixelRatio);

    bool Initialize();

    void Terminate();

    void PollEvents() const;

    [[nodiscard]] bool IsRunning() const;

    [[nodiscard]] GLFWwindow* GetWindow() const;

    void GetDrawableSize(int& width, int& height) const;

private:
    int         m_windowWidth  = 640;
    int         m_windowHeight = 480;
    double      m_maxDevicePixelRatio = 2.0;
    GLFWwindow* m_window       = nullptr;
};

#endif // BJTU_WGPU_RENDERER_WINDOWCONTEXT_H
