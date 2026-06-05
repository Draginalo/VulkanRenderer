#pragma once

#include <iostream>
#include "vulkan/vulkan.h"

bool createTextureImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkImage& textureImage, 
	VkDeviceMemory& textureImageMemory, VkCommandPool commandPool, VkQueue submitQueue, const char* imageFilename);

bool createImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, 
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask,
	VkPipelineStageFlags2 dstStageMask, void(*fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*));

void copyBufferToImage(VkDevice logicalDevice, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height,
	VkCommandPool commandPool, VkQueue submitQueue);