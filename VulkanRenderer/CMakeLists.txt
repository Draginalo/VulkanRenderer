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

# Builds the executable with all the header and source files in this directory
add_executable (VulkanRenderer ${VULKAN_RENDERER_SRC} ${VULKAN_RENDERER_INC})

# Adds vulcan libraries
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Vulkan)

# Adds glm and glfw libraries
target_link_libraries(VulkanRenderer PUBLIC glfw glm stb_image)

#Adds the compiled shaders as a dependency (generates the encoded shaders at build time)
add_dependencies(VulkanRenderer COMPILED_SHADERS)