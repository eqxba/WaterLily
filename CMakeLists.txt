cmake_minimum_required(VERSION 3.20)

set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} 8)

set(TARGET_NAME WaterLily)
project(${TARGET_NAME})

# sources
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SOURCE_FILES LIST_DIRECTORIES false
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.h"
)

# vulkan
find_package(Vulkan REQUIRED)

# glfw
add_subdirectory(External/glfw)

# glslang
option(ENABLE_SPVREMAPPER "" OFF)
option(ENABLE_GLSLANG_BINARIES "" OFF)
option(ENABLE_CTEST "" OFF)
option(ENABLE_HLSL "" OFF)
add_subdirectory(External/glslang)

# imgui
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/External/imgui")

file(GLOB IMGUI_HEADERS LIST_DIRECTORIES false
    "${IMGUI_DIR}/misc/cpp/*.h"
    "${IMGUI_DIR}/examples/*glfw*.h"
    "${IMGUI_DIR}/examples/*vulkan*.h"
)
file(GLOB IMGUI_SOURCES LIST_DIRECTORIES false
    "${IMGUI_DIR}/*.cpp"
    "${IMGUI_DIR}/misc/cpp/*.cpp"
    "${IMGUI_DIR}/backends/*glfw*.cpp"
    "${IMGUI_DIR}/backends/*vulkan*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_FILES} ${IMGUI_HEADERS} ${IMGUI_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    External/glfw/include/
    External/imgui/
    External/glslang/
    External/glslang/glslang/Include
    External/glslang/glslang/Public
    # This ones are header only :)
    External/VulkanMemoryAllocator/include
    External/glm/
    External/volk/
    External/tinygltf
    External/tinyobjloader
    External/portable-file-dialogs
    Source/
)

target_link_libraries(${TARGET_NAME} 
    PRIVATE ${Vulkan_LIBRARIES} 
    PRIVATE glfw
    PRIVATE glslang
)

target_precompile_headers(${TARGET_NAME} PUBLIC Source/pch.hpp)

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

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/Source/Shaders $<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/Assets $<TARGET_FILE_DIR:${TARGET_NAME}>/Assets
    COMMENT "Creating symlinks to Shaders and Assets directories..."
)

# IDE structuring
source_group("Source\\External\\Imgui" FILES ${IMGUI_HEADERS})
source_group("Source\\External\\Imgui\\Private" FILES ${IMGUI_SOURCES})
foreach(SOURCE IN ITEMS ${SOURCE_FILES})
    cmake_path(GET SOURCE PARENT_PATH PARENT_FOLDER)
    cmake_path(RELATIVE_PATH PARENT_FOLDER BASE_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE GROUP_PATH)
    source_group("${GROUP_PATH}" FILES "${SOURCE}")
endforeach()