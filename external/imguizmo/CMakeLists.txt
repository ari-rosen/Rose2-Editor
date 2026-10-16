cmake_minimum_required(VERSION 3.10)
project(ImGuizmo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imguizmo STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo.cpp
)

target_include_directories(imguizmo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
