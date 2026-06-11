cmake_minimum_required(VERSION 3.16)
project(CellkernelqwenOS VERSION 1.0.0 LANGUAGES C CXX Python)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
if(WIN32)
    set(PLATFORM win32)
elseif(APPLE)
    set(PLATFORM darwin)
else()
    set(PLATFORM linux)
endif()

# Options
option(ENABLE_UI "Enable UI framework" ON)
option(ENABLE_AUDIO "Enable audio subsystem" ON)
option(ENABLE_NETWORK "Enable network stack" ON)
option(ENABLE_FILESYSTEM "Enable filesystem" ON)

# Python integration
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# UI Framework
if(ENABLE_UI)
    if(PLATFORM STREQUAL "linux")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        include_directories(${GTK3_INCLUDE_DIRS})
        link_directories(${GTK3_LIBRARY_DIRS})
    endif()
    add_library(cellkernel-ui SHARED src/ui/interface.cpp)
    target_include_directories(cellkernel-ui PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# Audio Subsystem
if(ENABLE_AUDIO)
    add_library(cellkernel-audio SHARED src/audio/subsystem.cpp)
    target_include_directories(cellkernel-audio PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# Network Stack
if(ENABLE_NETWORK)
    add_library(cellkernel-network SHARED src/network/stack.cpp)
    target_include_directories(cellkernel-network PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# Filesystem
if(ENABLE_FILESYSTEM)
    add_library(cellkernel-fs SHARED src/filesystem/vfs.cpp)
    target_include_directories(cellkernel-fs PRIVATE ${Python3_INCLUDE_DIRS})
endif()

# Main Kernel
add_executable(cellkernelqwen-kernel src/kernel/main.cpp)
target_include_directories(cellkernelqwen-kernel PRIVATE 
    ${Python3_INCLUDE_DIRS}
    src/
)
target_link_libraries(cellkernelqwen-kernel PRIVATE ${Python3_LIBRARIES})

if(ENABLE_UI)
    target_link_libraries(cellkernelqwen-kernel PRIVATE cellkernel-ui)
endif()
if(ENABLE_AUDIO)
    target_link_libraries(cellkernelqwen-kernel PRIVATE cellkernel-audio)
endif()
if(ENABLE_NETWORK)
    target_link_libraries(cellkernelqwen-kernel PRIVATE cellkernel-network)
endif()
if(ENABLE_FILESYSTEM)
    target_link_libraries(cellkernelqwen-kernel PRIVATE cellkernel-fs)
endif()

# Tests
enable_testing()
add_executable(cellkernel-tests tests/test_runner.cpp)
add_test(NAME CellkernelTests COMMAND cellkernel-tests)

# Installation
install(TARGETS cellkernelqwen-kernel DESTINATION bin)
install(DIRECTORY src/ DESTINATION share/cellkernel)

# Build info
message(STATUS "Platform: ${PLATFORM}")
message(STATUS "UI: ${ENABLE_UI}")
message(STATUS "Audio: ${ENABLE_AUDIO}")
message(STATUS "Network: ${ENABLE_NETWORK}")
message(STATUS "Filesystem: ${ENABLE_FILESYSTEM}")
