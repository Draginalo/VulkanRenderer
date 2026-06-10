cmake_minimum_required(VERSION 3.15)

# Finds vulcan header
find_package(Vulkan REQUIRED)

# Gets all header files under the directory this file is in
file(
	GLOB RECURSE VULKAN_RENDERER_INC CONFIGURE_DEPENDS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	*.h *.hpp
)

# Gets all source (cpp) files under the directory this file is in
file(
 GLOB_RECURSE VULKAN_RENDERER_SRC CONFIGURE_DEPENDS
 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
 *.c *.cpp
)

include(${CMAKE_CURRENT_LIST_DIR}/../externals/compileBytecodeShaders.cmake)

#Copies asset folder to bin
add_custom_target(copyAssetsFP ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
${CMAKE_CURRENT_SOURCE_DIR}/../Assets/
${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../Assets/)

# Builds the executable with all the header and source files in this directory
add_executable (VulkanRenderer ${VULKAN_RENDERER_SRC} ${VULKAN_RENDERER_INC})

# Adds vulcan libraries
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Vulkan)

# Adds glm, glfw, and external header libraries
target_link_libraries(VulkanRenderer PUBLIC glfw glm IMGUI external_headers)

#Adds the compiled shaders and assets as dependencies (generates the encoded shaders at build time)
add_dependencies(VulkanRenderer COMPILED_SHADERS copyAssetsFP)