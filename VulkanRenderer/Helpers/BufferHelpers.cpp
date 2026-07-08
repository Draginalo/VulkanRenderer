#include "BufferHelpers.hpp"

bool createBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create vertex buffer..." << std::endl;
		return false;
	}

	VkMemoryRequirements memoryRequirments{};
	vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirments);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirments.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirments.memoryTypeBits,
		properties);

	//TODO: Do not allocate memory for each individual buffer. Refactor to allocate for many different objects with a 
	// single allocation using offset parameters (could use https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
	// library)
	if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate memory for vertex buffer..." << std::endl;
		return false;
	}

	vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);

	return true;
}

bool copyBuffer(VkDevice logicalDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
	VkCommandPool commandPool, VkQueue submitQueue)
{
	VkCommandBuffer commandBuffer;
	if (!beginSingleTimeCommandBuffer(logicalDevice, &commandBuffer, commandPool))
	{
		std::cout << "\nFailed to begin command buffer for buffer copy..." << std::endl;
		return false;
	}

	VkBufferCopy copyRegion{0, 0};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	if (!endSingleTimeCommandBuffer(logicalDevice, commandBuffer, commandPool, submitQueue))
	{
		std::cout << "\nFailed to end command buffer for buffer copy..." << std::endl;
		return false;
	}

	return true;
}

bool beginSingleTimeCommandBuffer(VkDevice logicalDevice, VkCommandBuffer* pCommandBuffer, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandPool = commandPool;
	cmdBufferAllocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(logicalDevice, &cmdBufferAllocateInfo, pCommandBuffer);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(*pCommandBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool endSingleTimeCommandBuffer(VkDevice logicalDevice, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue submitQueue)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end single time command buffer..." << std::endl;
		return false;
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		std::cout << "\nFailed to submit single time command buffer to queue..." << std::endl;
		return false;
	}

	vkQueueWaitIdle(submitQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);

	return true;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; (i < memoryProperties.memoryTypeCount) == true; i++)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type...");
	//return UINT32_MAX;
}