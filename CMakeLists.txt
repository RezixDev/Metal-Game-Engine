cmake_minimum_required(VERSION 3.20)

project(MetalEngine VERSION 1.0.0 LANGUAGES CXX C OBJCXX OBJC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT APPLE)
    message(FATAL_ERROR "MetalEngine requires macOS")
endif()

enable_language(OBJC)
enable_language(OBJCXX)

find_library(COCOA_FRAMEWORK      Cocoa      REQUIRED)
find_library(METAL_FRAMEWORK      Metal      REQUIRED)
find_library(METALKIT_FRAMEWORK   MetalKit   REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        MetalEngine/main.m
        MetalEngine/AppDelegate.m
        MetalEngine/MyMetalView.mm
        MetalEngine/MetalRenderer.mm
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        OBJCXX_STANDARD 17
        CXX_STANDARD 17
        LINKER_LANGUAGE OBJCXX
)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>
)

# ===================================
# Metal Shader Compilation
# ===================================

# Compile Shaders.metal into default.metallib so [MTLDevice newDefaultLibrary]
# finds it inside the app bundle.
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        COMMAND xcrun -sdk macosx metal -gline-tables-only -frecord-sources -c
                ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Shaders.metal
                -o ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air
        COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air
                -o ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Shaders.metal
        COMMENT "Compiling Metal shaders into default.metallib"
)

add_custom_target(CompileShaders ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
)

add_dependencies(${PROJECT_NAME} CompileShaders)

# ===================================
# Copy resources to bundle
# ===================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Assets.xcassets)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Assets.xcassets
            $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/Assets.xcassets
    )
endif()

add_custom_target(run
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL
        COMMENT "Running ${PROJECT_NAME}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0 -DDEBUG)
endif()
