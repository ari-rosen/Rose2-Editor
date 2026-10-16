cmake_minimum_required(VERSION 3.20)
project(RoseV2Editor VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Output Directories --- #
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Vendor/External Directories --- #
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# --- Dependencies --- #
# RoseV2 Library (already handles GLFW + OpenGL)
add_subdirectory(../RoseV2 RoseV2Build)

# ImGui (docking branch)
file(GLOB IMGUI_SOURCES
    ${EXTERNAL_DIR}/imgui/*.cpp
    ${EXTERNAL_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${EXTERNAL_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${EXTERNAL_DIR}/imgui
    ${EXTERNAL_DIR}/imgui/backends
)

add_subdirectory(./external/nativefiledialog)
add_subdirectory(./external/imguizmo)

target_link_libraries(imgui PUBLIC RoseV2)

target_link_libraries(imguizmo PUBLIC imgui)

file(GLOB_RECURSE EDITOR_SOURCES
    src/*.cpp
)

add_executable(RoseV2Editor ${EDITOR_SOURCES})

target_include_directories(RoseV2Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${EXTERNAL_DIR}/tinyfiledialogs
)

target_link_libraries(RoseV2Editor PRIVATE RoseV2 nfd imguizmo)

# --- Platform-specific Linking (macOS/Unix) --- #
if(APPLE)
    target_link_libraries(RoseV2Editor PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Foundation"
        "-framework AppKit"
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(RoseV2Editor PRIVATE OpenGL::GL)
endif()

target_compile_definitions(RoseV2Editor PRIVATE 
    EDITOR_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
