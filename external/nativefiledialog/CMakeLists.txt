cmake_minimum_required(VERSION 3.10)
project(nfd C CXX)

set(NFD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(nfd STATIC
    ${NFD_SRC_DIR}/nfd_common.c
)

if (APPLE)
    target_sources(nfd PRIVATE "${NFD_SRC_DIR}/nfd_cocoa.m")
    target_link_libraries(nfd PRIVATE "-framework Cocoa")
elseif (WIN32)
    target_sources(nfd PRIVATE "${NFD_SRC_DIR}/nfd_win.cpp")
elseif (UNIX AND NOT APPLE)
    target_sources(nfd PRIVATE "${NFD_SRC_DIR}/nfd_gtk.c")
    # You will need to link the appropriate GTK libraries here.
    # find_package(GTK REQUIRED)
    # target_link_libraries(nfd PRIVATE GTK::GTK)
endif()

# Set a public include directory so other projects can find nfd.h
target_include_directories(nfd PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
