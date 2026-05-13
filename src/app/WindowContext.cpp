#include "WindowContext.h"

#include <algorithm>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#ifdef __EMSCRIPTEN__
namespace {
double ClampDevicePixelRatio(const double deviceDpr, const double maxDevicePixelRatio) {
    return std::clamp(deviceDpr, 1.0, std::max(1.0, maxDevicePixelRatio));
}

void ConfigureCanvasForHighDpi(const int windowWidth, const int windowHeight, const double maxDevicePixelRatio) {
    emscripten_set_element_css_size("#canvas", static_cast<double>(windowWidth), static_cast<double>(windowHeight));

    double cssWidth  = static_cast<double>(windowWidth);
    double cssHeight = static_cast<double>(windowHeight);
    if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || cssWidth <= 0.0 || cssHeight <= 0.0) {
        cssWidth  = static_cast<double>(windowWidth);
        cssHeight = static_cast<double>(windowHeight);
    }

    const double dpr      = ClampDevicePixelRatio(emscripten_get_device_pixel_ratio(), maxDevicePixelRatio);
    const int    pixelWidth  = std::max(1, static_cast<int>(std::lround(cssWidth * dpr)));
    const int    pixelHeight = std::max(1, static_cast<int>(std::lround(cssHeight * dpr)));
    emscripten_set_canvas_element_size("#canvas", pixelWidth, pixelHeight);
}

void UpdateCanvasBackingStoreForCurrentDpr(const double maxDevicePixelRatio) {
    double cssWidth  = 0.0;
    double cssHeight = 0.0;
    if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || cssWidth <= 0.0 || cssHeight <= 0.0) {
        return;
    }

    const double dpr          = ClampDevicePixelRatio(emscripten_get_device_pixel_ratio(), maxDevicePixelRatio);
    const int    desiredWidth  = std::max(1, static_cast<int>(std::lround(cssWidth * dpr)));
    const int    desiredHeight = std::max(1, static_cast<int>(std::lround(cssHeight * dpr)));

    int currentWidth  = 0;
    int currentHeight = 0;
    if (emscripten_get_canvas_element_size("#canvas", &currentWidth, &currentHeight) != EMSCRIPTEN_RESULT_SUCCESS
        || currentWidth != desiredWidth || currentHeight != desiredHeight) {
        emscripten_set_canvas_element_size("#canvas", desiredWidth, desiredHeight);
    }
}
} // namespace
#endif

WindowContext& WindowContext::SetWindowSize(const int width, const int height) {
    m_windowWidth  = width;
    m_windowHeight = height;
    return *this;
}

WindowContext& WindowContext::SetMaxDevicePixelRatio(const double maxDevicePixelRatio) {
    if (!std::isfinite(maxDevicePixelRatio)) {
        m_maxDevicePixelRatio = 2.0;
        return *this;
    }
    m_maxDevicePixelRatio = std::max(1.0, maxDevicePixelRatio);
    return *this;
}

bool WindowContext::Initialize() {
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "WebGPU Renderer", nullptr, nullptr);
    if (m_window == nullptr) {
        glfwTerminate();
        return false;
    }

#ifdef __EMSCRIPTEN__
    ConfigureCanvasForHighDpi(m_windowWidth, m_windowHeight, m_maxDevicePixelRatio);
#endif
    return true;
}

void WindowContext::Terminate() {
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void WindowContext::PollEvents() const {
#ifdef __EMSCRIPTEN__
    UpdateCanvasBackingStoreForCurrentDpr(m_maxDevicePixelRatio);
#endif
    glfwPollEvents();
}

bool WindowContext::IsRunning() const {
    return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

GLFWwindow* WindowContext::GetWindow() const {
    return m_window;
}

void WindowContext::GetDrawableSize(int& width, int& height) const {
    width  = 0;
    height = 0;

#ifdef __EMSCRIPTEN__
    if (emscripten_get_canvas_element_size("#canvas", &width, &height) == EMSCRIPTEN_RESULT_SUCCESS
        && width > 0 && height > 0) {
        return;
    }
#endif

    if (m_window != nullptr) {
        glfwGetFramebufferSize(m_window, &width, &height);
    }
    if (width <= 0) {
        width = m_windowWidth;
    }
    if (height <= 0) {
        height = m_windowHeight;
    }
}
