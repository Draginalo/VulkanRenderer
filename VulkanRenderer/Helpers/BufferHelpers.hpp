#pragma once

#include <vulkan/vulkan.h>
#include <iostream>

bool createBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

bool copyBuffer(VkDevice logicalDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
	VkCommandPool commandPool, VkQueue submitQueue);

bool beginSingleTimeCommandBuffer(VkDevice logicalDevice, VkCommandBuffer* pCommandBuffer, VkCommandPool commandPool);
bool endSingleTimeCommandBuffer(VkDevice logicalDevice, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue submitQueue);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);