#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <array>
#include <iostream>

#include "VulkanRenderer/Helpers/VertexInputData.h"
#include "Drawable.h"

class MeshGeneric : public Drawable {
public:
	~MeshGeneric() {}

	bool createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
		VkQueue submitQueue, const void* vertexData, VkDeviceSize vertexBufferSize, std::vector<uint32_t> indecies, 
		VertexInputData vertexInputData);

	bool createIndeciesBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		VkQueue submitQueue);

	void cleanupBuffers(VkDevice logicalDevice);

	void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const override;
	VkBuffer getLastFrameVertexBuffer() const override;

	const VertexInputData getVertexInputData() const;
protected:
	VkBuffer mVertexBuffer = VK_NULL_HANDLE;
	VkBuffer mIndeciesBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
	VkDeviceMemory mIndeciesBufferMemory = VK_NULL_HANDLE;

	VertexInputData mVertexInputData{};
	std::vector<uint32_t> mIndecies;

	//Explicit padding for x86 since struct size is not divisable by 8 in x86
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[4];
	#endif
};