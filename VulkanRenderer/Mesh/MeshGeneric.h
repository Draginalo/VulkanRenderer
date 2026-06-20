#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "vulkan/vulkan.h"
#include <array>
#include <iostream>

#include "../Helpers/VertexInputData.h"
#include "Drawable.h"

class MeshGeneric : public Drawable {
public:
	bool createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
		VkQueue submitQueue, const void* vertexData, VkDeviceSize vertexBufferSize, std::vector<uint32_t> indecies, 
		VertexInputData vertexInputData);

	bool createIndeciesBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		VkQueue submitQueue);

	void cleanupBuffers(VkDevice logicalDevice);

	void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const override;
	const VkBuffer getLastFrameVertexBuffer() const override;

	inline VertexInputData getVertexInputData() { return mVertexInputData; }
protected:
	std::vector<uint32_t> mIndecies;

	VkBuffer mVertexBuffer;
	VkBuffer mIndeciesBuffer;
	VkDeviceMemory mVertexBufferMemory;
	VkDeviceMemory mIndeciesBufferMemory;

	VertexInputData mVertexInputData;
};