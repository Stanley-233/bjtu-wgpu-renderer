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

add_executable(bjtu_wgpu_renderer src/main.cpp
        src/utils.cpp
        src/utils.h
        src/application.cpp
        src/application.h)

target_link_libraries(bjtu_wgpu_renderer PRIVATE webgpu glfw glfw3webgpu)

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
    # Enable the use of emscripten_sleep()
    target_link_options(bjtu_wgpu_renderer PRIVATE -sASYNCIFY)
endif()
