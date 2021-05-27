cmake_minimum_required(VERSION 3.20)

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

# IDE structuring
# TODO: Still not sure about naming (case & ...)
foreach(source IN ITEMS ${SOURCE_FILES})
    cmake_path(GET source PARENT_PATH parent_folder)
    cmake_path(RELATIVE_PATH parent_folder BASE_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE group_path)
    source_group("${group_path}" FILES "${source}")
endforeach()

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${TARGET_NAME} PROPERTY CMAKE_CXX_STANDARD_REQUIRED True)

target_include_directories(${TARGET_NAME} PRIVATE    
    Source/
)