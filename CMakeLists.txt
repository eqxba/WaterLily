cmake_minimum_required(VERSION 3.20)

set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} 8)

set(TARGET_NAME WaterLily)
project(${TARGET_NAME})

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
# TODO: GLOB sometimes is thought as a bad solution to add source files (change this to a better way?)
file(GLOB_RECURSE SOURCE_FILES LIST_DIRECTORIES false
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
)

add_executable(${TARGET_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)

add_subdirectory(External/glfw)

target_include_directories(${TARGET_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    External/glfw/include/
    # This ones are header only :)
    External/VulkanMemoryAllocator/include
    External/glm/
    External/volk/
    External/tinygltf
    External/tinyobjloader
    Source/
)

target_link_libraries(${TARGET_NAME} ${Vulkan_LIBRARIES} glfw)

# IDE structuring
# TODO: Still not sure about naming (case & ...)
foreach(source IN ITEMS ${SOURCE_FILES})
    cmake_path(GET source PARENT_PATH parent_folder)
    cmake_path(RELATIVE_PATH parent_folder BASE_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE group_path)
    source_group("${group_path}" FILES "${source}")
endforeach()

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${TARGET_NAME} PROPERTY CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
    add_compile_definitions(PLATFORM_WIN)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX /MP)
else()
    add_compile_definitions(PLATFORM_MAC)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_precompile_headers(${TARGET_NAME} PUBLIC Source/pch.hpp)

# TODO: Make CMake compile shaders for us with a custom command
