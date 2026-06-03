#include "VertexBufferData.h"

bool VertexBufferData::createBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
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

bool VertexBufferData::copyBuffer(VkDevice logicalDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, 
VkCommandPool commandPool, VkQueue submitQueue)
{
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandPool = commandPool;
	cmdBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &cmdBufferAllocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo) != VK_SUCCESS)
	{
		std::cout << "\nFailed to begin command buffer for buffer copy..." << std::endl;
		return false;
	}

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end command buffer for buffer copy..." << std::endl;
		return false;
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) 
	{
		std::cout << "\nFailed to submit command buffer to queue for buffer copy..." << std::endl;
		return false;
	}

	vkQueueWaitIdle(submitQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);

	return true;
}

bool VertexBufferData::createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
	VkQueue submitQueue)
{
	VkDeviceSize bufferSize = sizeof(mVerticies[0]) * mVerticies.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory))
	{
		return false;
	}

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mVerticies.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer, mVertexBufferMemory))
	{
		return false;
	}

	//Copies vertex data from cpu staging buffer to gpu local memory buffer
	if (!copyBuffer(logicalDevice, stagingBuffer, mVertexBuffer, bufferSize, commandPool, submitQueue))
	{
		return false;
	}

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

	return true;
}

bool VertexBufferData::createIndeciesBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue submitQueue)
{
	VkDeviceSize bufferSize = sizeof(mIndecies[0]) * mIndecies.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory))
	{
		return false;
	}

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mIndecies.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndeciesBuffer, mIndeciesBufferMemory))
	{
		return false;
	}

	//Copies vertex data from cpu staging buffer to gpu local memory buffer
	if (!copyBuffer(logicalDevice, stagingBuffer, mIndeciesBuffer, bufferSize, commandPool, submitQueue))
	{
		return false;
	}

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

	return true;
}

uint32_t VertexBufferData::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter && (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type...");
	return UINT32_MAX;
}

void VertexBufferData::cleanupBuffers(VkDevice logicalDevice)
{
	vkDestroyBuffer(logicalDevice, mVertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, mVertexBufferMemory, nullptr);

	vkDestroyBuffer(logicalDevice, mIndeciesBuffer, nullptr);
	vkFreeMemory(logicalDevice, mIndeciesBufferMemory, nullptr);
}

void VertexBufferData::draw(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { mVertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, mIndeciesBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mIndecies.size()), 1, 0, 0, 0);
}
