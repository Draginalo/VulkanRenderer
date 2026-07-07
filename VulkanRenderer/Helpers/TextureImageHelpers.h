#pragma once

#include <iostream>
#include "vulkan/vulkan.h"

bool createTextureImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkImage& textureImage, 
	VkDeviceMemory& textureImageMemory, VkCommandPool commandPool, VkQueue submitQueue, const char* imageFilename, 
	uint32_t& mipLevels);

void generateMipMaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, 
	uint32_t mipLevels, VkPhysicalDevice physicalDevice);

bool createImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, 
	VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory);

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask,
	VkPipelineStageFlags2 dstStageMask, VkImageAspectFlags aspectMask, uint32_t mipLevels,
	void(__stdcall *fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*) = nullptr);

void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height,
	uint32_t mipLevels, VkCommandBuffer commandBuffer);

bool createImageView(VkDevice logicalDevice, VkImage image, VkImageView* pImageView, VkFormat format, VkImageAspectFlags aspectFlags,
	uint32_t mipLevels);

bool createTextureSampler(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkSampler* pTextureSampler);