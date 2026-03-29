cmake_minimum_required(VERSION 4.1)
project(
    bjtu_wgpu_renderer
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Add glfw
if (NOT EMSCRIPTEN)
    add_subdirectory(ext/glfw)
else()
    # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(ext/webgpu)

add_subdirectory(ext/glfw3webgpu)

add_subdirectory(ext/magic_enum)

add_subdirectory(ext/toml++)

add_subdirectory(ext/glm)

add_subdirectory(ext/tiny_obj_loader)

add_subdirectory(ext/entt)

add_executable(bjtu_wgpu_renderer
        src/main.cpp
        src/app/Application.cpp
        src/app/Application.h
        src/input/InputManager.cpp
        src/input/InputManager.h
        src/input/InputEventLogger.cpp
        src/input/InputEventLogger.h
        src/render/RenderContext.cpp
        src/render/RenderContext.h
        src/render/PipelineLibrary.cpp
        src/render/PipelineLibrary.h
        src/scene/Scene.h
        src/scene/SceneManager.cpp
        src/scene/SceneManager.h
        src/scene/Scene2D.cpp
        src/scene/Scene2D.h
        src/math/Transform2D.cpp
        src/math/Transform2D.h
        src/resource/ResourceManager.cpp
        src/resource/ResourceManager.h
)

target_compile_definitions(bjtu_wgpu_renderer PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(bjtu_wgpu_renderer PRIVATE
        webgpu glfw glfw3webgpu
        magic_enum
        toml
        glm
        tiny_obj_loader
        entt
)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(bjtu_wgpu_renderer)

set_target_properties(bjtu_wgpu_renderer PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(bjtu_wgpu_renderer PRIVATE /W4)
else()
    target_compile_options(bjtu_wgpu_renderer PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
    set_target_properties(bjtu_wgpu_renderer PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

# Options that are specific to Emscripten
if (EMSCRIPTEN)
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(bjtu_wgpu_renderer PROPERTIES SUFFIX ".html")
    target_link_options(bjtu_wgpu_renderer PRIVATE
            -sASYNCIFY
            --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
endif()
