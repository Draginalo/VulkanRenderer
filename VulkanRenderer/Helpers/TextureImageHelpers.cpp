#include "TextureImageHelpers.h"

#include "BufferHelpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool createTextureImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkImage& textureImage, 
	VkDeviceMemory& textureImageMemory, VkCommandPool commandPool, VkQueue submitQueue, const char* imageFilename, 
	uint32_t& mipLevels)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(imageFilename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	//Multiplied by 4 because of the 4 data channels (RGBA)
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		std::cout << "\nFailed to load image file: " << imageFilename << "..." << std::endl;
		return false;
	}

	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;

	if (!createBuffer(logicalDevice, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stageBuffer, stageBufferMemory))
	{
		return false;
	}

	void* data;
	vkMapMemory(logicalDevice, stageBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(logicalDevice, stageBufferMemory);

	stbi_image_free(pixels);

	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))) + 1);
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	if (!createImage(logicalDevice, physicalDevice, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, imageFormat, 
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory)) 
	{
		std::cout << "\nFailed to create texture image..." << std::endl;
		return false;
	}

	VkCommandBuffer commandBuffer;
	if (!beginSingleTimeCommandBuffer(logicalDevice, &commandBuffer, commandPool)) { return false; }

	transitionImageLayout(commandBuffer, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, nullptr);

	copyBufferToImage(logicalDevice, stageBuffer, textureImage, texWidth, texHeight, mipLevels, commandBuffer);

	generateMipMaps(commandBuffer, textureImage, imageFormat, texWidth, texHeight, mipLevels, physicalDevice);

	if (!endSingleTimeCommandBuffer(logicalDevice, commandBuffer, commandPool, submitQueue)) { return false; }

	vkDestroyBuffer(logicalDevice, stageBuffer, nullptr);
	vkFreeMemory(logicalDevice, stageBufferMemory, nullptr);

	return true;
}

void generateMipMaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, 
	uint32_t mipLevels, VkPhysicalDevice physicalDevice)
{
	VkFormatProperties formatProps{};
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("Texture image format does not support linear blitting");
	}

	VkImageSubresourceRange subResourceRange{};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.layerCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.levelCount = 1;

	VkImageMemoryBarrier imageMemoryBarrierInfo{};
	imageMemoryBarrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrierInfo.image = image;
	imageMemoryBarrierInfo.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrierInfo.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrierInfo.subresourceRange = subResourceRange;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (int i = 1; i < mipLevels; i++)
	{
		imageMemoryBarrierInfo.subresourceRange.baseMipLevel = i - 1;
		imageMemoryBarrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrierInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrierInfo.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierInfo);

		VkImageBlit blitInfo{};
		blitInfo.srcOffsets[0] = { 0, 0, 0 };
		blitInfo.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blitInfo.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitInfo.srcSubresource.mipLevel = i - 1;
		blitInfo.srcSubresource.baseArrayLayer = 0;
		blitInfo.srcSubresource.layerCount = 1;
		blitInfo.dstOffsets[0] = {0, 0, 0};
		blitInfo.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blitInfo.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitInfo.dstSubresource.mipLevel = i;
		blitInfo.dstSubresource.baseArrayLayer = 0;
		blitInfo.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blitInfo, VK_FILTER_LINEAR);

		imageMemoryBarrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrierInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierInfo);

		if (mipWidth > 1) { mipWidth /= 2; }
		if (mipHeight > 1) { mipHeight /= 2; }
	}

	//Transfers layout of last mip level (since loop executes until one less than mipLevels where i = mipLevels - 1)
	imageMemoryBarrierInfo.subresourceRange.baseMipLevel = mipLevels - 1;
	imageMemoryBarrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrierInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrierInfo);
}

bool createImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, 
	VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = numSamples;

	if (vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create image..." << std::endl;
		return false;
	}

	VkMemoryRequirements memRequirments;
	vkGetImageMemoryRequirements(logicalDevice, image, &memRequirments);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirments.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirments.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate image memory..." << std::endl;
		return false;
	}

	vkBindImageMemory(logicalDevice, image, imageMemory, 0);

	return true;
}

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask,
	VkPipelineStageFlags2 dstStageMask, VkImageAspectFlags aspectMask, uint32_t mipLevels,
	void(*fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*))
{
	if (fpCmdPipelineBarrier2 != nullptr) {
		VkImageSubresourceRange subResourceRange{};
		subResourceRange.aspectMask = aspectMask;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.layerCount = 1;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.levelCount = mipLevels;

		VkImageMemoryBarrier2 imageMemoryBarrierInfo{};
		imageMemoryBarrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageMemoryBarrierInfo.image = image;
		imageMemoryBarrierInfo.oldLayout = oldLayout;
		imageMemoryBarrierInfo.newLayout = newLayout;
		imageMemoryBarrierInfo.srcAccessMask = srcAccessMask;
		imageMemoryBarrierInfo.dstAccessMask = dstAccessMask;
		imageMemoryBarrierInfo.srcStageMask = srcStageMask;
		imageMemoryBarrierInfo.dstStageMask = dstStageMask;
		imageMemoryBarrierInfo.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrierInfo.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrierInfo.subresourceRange = subResourceRange;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrierInfo;

		fpCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

		return;
	}

	//Using old vkCmdPipelineBarrier if extension function pointer is not defined
	VkImageSubresourceRange subResourceRange{};
	subResourceRange.aspectMask = aspectMask;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = mipLevels;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	VkImageMemoryBarrier imageMemoryBarrierInfo{};
	imageMemoryBarrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrierInfo.image = image;
	imageMemoryBarrierInfo.oldLayout = oldLayout;
	imageMemoryBarrierInfo.newLayout = newLayout;
	imageMemoryBarrierInfo.srcAccessMask = srcAccessMask;
	imageMemoryBarrierInfo.dstAccessMask = dstAccessMask;
	imageMemoryBarrierInfo.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrierInfo.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrierInfo.subresourceRange = subResourceRange;

	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierInfo);
}

void copyBufferToImage(VkDevice logicalDevice, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height, 
	uint32_t mipLevels, VkCommandBuffer commandBuffer)
{
	VkBufferImageCopy imageRegion{};
	imageRegion.bufferOffset = 0;
	imageRegion.bufferRowLength = 0;
	imageRegion.bufferImageHeight = 0;
	imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageRegion.imageSubresource.mipLevel = 0;
	imageRegion.imageSubresource.baseArrayLayer = 0;
	imageRegion.imageSubresource.layerCount = 1;
	imageRegion.imageOffset = { 0, 0, 0 };
	imageRegion.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
}

bool createImageView(VkDevice logicalDevice, VkImage image, VkImageView* pImageView, VkFormat format, VkImageAspectFlags aspectFlags,
	uint32_t mipLevels)
{
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, pImageView) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create image view..." << std::endl;
		return false;
	}

	return true;
}
