#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "vulkan/vulkan.h"
#include <array>
#include <iostream>

struct Vertex {
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

class VertexBufferData {
public:
	static bool createBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	bool copyBuffer(VkDevice logicalDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, 
VkCommandPool commandPool, VkQueue submitQueue);

	bool createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
		VkQueue submitQueue);

	bool createIndeciesBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		VkQueue submitQueue);

	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void cleanupBuffers(VkDevice logicalDevice);

	void draw(VkCommandBuffer commandBuffer);
private:
	std::vector<Vertex> mVerticies = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> mIndecies = {
		0, 1, 2, 2, 3, 0
	};

	VkBuffer mVertexBuffer;
	VkBuffer mIndeciesBuffer;
	VkDeviceMemory mVertexBufferMemory;
	VkDeviceMemory mIndeciesBufferMemory;
};