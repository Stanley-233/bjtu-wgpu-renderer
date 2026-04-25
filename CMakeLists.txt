cmake_minimum_required(VERSION 3.31.6)
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

add_subdirectory(ext/imgui)

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
        src/render/Renderer3D.cpp
        src/render/Renderer3D.h
        src/scene/Scene.h
        src/scene/SceneManager.cpp
        src/scene/SceneManager.h
        src/scene/Scene2D.cpp
        src/scene/Scene2D.h
        src/scene/scene3d/Scene3D.cpp
        src/scene/scene3d/Scene3D.h
        src/scene/camera/Camera.cpp
        src/scene/camera/Camera.h
        src/scene/camera/PerspectiveCamera.cpp
        src/scene/camera/PerspectiveCamera.h
        src/scene/camera/OrthographicCamera.cpp
        src/scene/camera/OrthographicCamera.h
        src/scene/scene3d/Object3D.cpp
        src/scene/scene3d/Object3D.h
        src/math/Transform2D.cpp
        src/math/Transform2D.h
        src/resource/models/MeshData3D.h
        src/resource/models/SceneDescription.h
        src/resource/loaders/IModelLoader.h
        src/resource/loaders/ObjLoader.cpp
        src/resource/loaders/ObjLoader.h
        src/resource/loaders/ISceneLoader.h
        src/resource/loaders/TomlSceneLoader.cpp
        src/resource/loaders/TomlSceneLoader.h
        src/resource/loaders/ShaderLoader.cpp
        src/resource/loaders/ShaderLoader.h
        src/resource/loaders/LegacyTxtGeometryLoader.cpp
        src/resource/loaders/LegacyTxtGeometryLoader.h
        src/resource/ResourceManager.cpp
        src/resource/ResourceManager.h
        src/math/Transform3D.h
        src/math/Transform3D.cpp
        src/resource/ResourcePaths.h
        src/resource/ResourcePaths.cpp
)

target_compile_definitions(bjtu_wgpu_renderer PRIVATE
        $<$<CONFIG:Debug>:RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources">
)

target_link_libraries(bjtu_wgpu_renderer PRIVATE
        webgpu glfw glfw3webgpu
        magic_enum
        toml
        glm
        tiny_obj_loader
        entt
        imgui
)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(bjtu_wgpu_renderer)

set(BJTU_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/package")
foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG_TYPE}" CONFIG_UPPER)
    set_target_properties(bjtu_wgpu_renderer PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${BJTU_PACKAGE_ROOT}/${CONFIG_TYPE}"
            LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${BJTU_PACKAGE_ROOT}/${CONFIG_TYPE}"
            ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${BJTU_PACKAGE_ROOT}/${CONFIG_TYPE}"
    )
endforeach()

if (NOT EMSCRIPTEN)
    add_custom_command(TARGET bjtu_wgpu_renderer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:bjtu_wgpu_renderer>/resources"
            COMMENT "Copying resources next to executable"
    )
endif()

if (APPLE)
    set_target_properties(bjtu_wgpu_renderer PROPERTIES
            BUILD_RPATH "@loader_path"
            INSTALL_RPATH "@loader_path"
    )
elseif (UNIX AND NOT EMSCRIPTEN)
    set_target_properties(bjtu_wgpu_renderer PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
    )
endif()

set_target_properties(bjtu_wgpu_renderer PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

if (MSVC)
    target_compile_options(bjtu_wgpu_renderer PRIVATE /W4)
else()
    target_compile_options(bjtu_wgpu_renderer PRIVATE -Wall -Wextra -pedantic)
    set_target_properties(bjtu_wgpu_renderer PROPERTIES COMPILE_WARNING_AS_ERROR ON)
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
            --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources"
    )
endif()
