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

add_subdirectory(ext/entt)

add_subdirectory(ext/imgui)

file(GLOB_RECURSE BJTU_RENDERER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_executable(bjtu_wgpu_renderer
        ${BJTU_RENDERER_SOURCES}
)

target_include_directories(bjtu_wgpu_renderer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tiny_obj_loader
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tiny_gltf
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/json
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb_image
)

target_compile_definitions(bjtu_wgpu_renderer PRIVATE
        $<$<CONFIG:Debug>:ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets">
        $<$<CONFIG:Debug>:LEGACY_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy">
        $<$<CONFIG:Debug>:LEGACY_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shader/legacy">
        $<$<CONFIG:Debug>:CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config">
)

target_link_libraries(bjtu_wgpu_renderer PRIVATE
        webgpu glfw glfw3webgpu
        magic_enum
        toml
        glm
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
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:bjtu_wgpu_renderer>/assets"
            COMMENT "Copying assets next to executable"
    )
    add_custom_command(TARGET bjtu_wgpu_renderer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shader"
            "$<TARGET_FILE_DIR:bjtu_wgpu_renderer>/shader"
            COMMENT "Copying shader resources next to executable"
    )
    add_custom_command(TARGET bjtu_wgpu_renderer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:bjtu_wgpu_renderer>/config"
            COMMENT "Copying config next to executable"
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
            --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
            --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/shader@/shader"
            --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/config@/config"
    )
endif()
