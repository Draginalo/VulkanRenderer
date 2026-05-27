cmake_minimum_required(VERSION 3.15)

# Finds vulcan header
find_package(Vulkan REQUIRED)


add_executable (VulkanRenderer "VulkanRenderer.cpp" "VulkanRenderer.h")

# Adds vulcan libraries
target_link_libraries(VulkanRenderer PRIVATE Vulkan::Vulkan)

# Adds glm libraries
target_link_libraries(VulkanRenderer PUBLIC glfw glm)