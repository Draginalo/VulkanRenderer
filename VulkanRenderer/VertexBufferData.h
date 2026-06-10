#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "vulkan/vulkan.h"
#include <array>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoords;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkVertexInputAttributeDescription description{};

		description.binding = 0;
		description.location = 0;
		description.format = VK_FORMAT_R32G32B32_SFLOAT;
		description.offset = offsetof(Vertex, position);

		attributeDescriptions.push_back(description);

		description.binding = 0;
		description.location = 1;
		description.format = VK_FORMAT_R32G32B32_SFLOAT;
		description.offset = offsetof(Vertex, color);

		attributeDescriptions.push_back(description);

		description.binding = 0;
		description.location = 2;
		description.format = VK_FORMAT_R32G32_SFLOAT;
		description.offset = offsetof(Vertex, texCoords);

		attributeDescriptions.push_back(description);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return position == other.position && color == other.color && texCoords == other.texCoords;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoords) << 1);
		}
	};
}

class VertexBufferData {
public:
	bool createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
		VkQueue submitQueue);

	bool createIndeciesBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		VkQueue submitQueue);

	void cleanupBuffers(VkDevice logicalDevice);

	void draw(VkCommandBuffer commandBuffer);
private:
	std::vector<Vertex> mVerticies;
	std::vector<uint32_t> mIndecies;

	VkBuffer mVertexBuffer;
	VkBuffer mIndeciesBuffer;
	VkDeviceMemory mVertexBufferMemory;
	VkDeviceMemory mIndeciesBufferMemory;
};