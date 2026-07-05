cmake_minimum_required(VERSION 3.15)

# Finds vulcan header
find_package(Vulkan REQUIRED)

# Gets all header files under the directory this file is in
file(
	GLOB RECURSE VULKAN_RENDERER_INC
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	*.h *.hpp
)

# Gets all source (cpp) files under the directory this file is in
file(
 GLOB_RECURSE VULKAN_RENDERER_SRC
 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
 *.c *.cpp
)

include(${PROJECT_SOURCE_DIR}/externals/compileBytecodeShaders.cmake)

#Copies asset folder to bin
add_custom_target(copyAssetsFP ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
${PROJECT_SOURCE_DIR}/Assets/
${PROJECT_SOURCE_DIR}/Assets/)

# Builds the executable with all the header and source files in this directory
add_executable (VulkanRenderer ${VULKAN_RENDERER_SRC} ${VULKAN_RENDERER_INC})

if (MSVC)
    set(COMPILE_OPTIONS
        "/Wall" # Enable warnings (level 4; use Wall for highest)
        "/WX" # Warnings as errors
        "/Zc:__cplusplus" # CPP predefined macro is version
        "/Zc:preprocessor" # Preprocessor uses standard
        "/external:W0"
        "/external:anglebrackets"
        "/wd5045"
    )

    target_compile_options(VulkanRenderer PRIVATE
        "${COMPILE_OPTIONS}"
    )
endif()

target_include_directories(VulkanRenderer PRIVATE "${PROJECT_SOURCE_DIR}")

# Adds vulcan libraries
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Vulkan)

# Adds glm, glfw, and external header libraries
target_link_libraries(VulkanRenderer PUBLIC glfw glm IMGUI external_headers)

#Adds the compiled shaders and assets as dependencies (generates the encoded shaders at build time)
add_dependencies(VulkanRenderer COMPILED_SHADERS copyAssetsFP)