#include "VertexBufferData.h"
#include "Helpers/BufferHelpers.h"
#include "Helpers/ModelHelpers.h"

bool VertexBufferData::createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
	VkQueue submitQueue)
{
	loadModel("../Assets/Models/Room/room.obj", &mVerticies,  &mIndecies);

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
	vkCmdBindIndexBuffer(commandBuffer, mIndeciesBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mIndecies.size()), 1, 0, 0, 0);
}
