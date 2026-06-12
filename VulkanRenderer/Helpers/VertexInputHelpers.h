#pragma once

#include "vulkan/vulkan.h"
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//Helper struct to bundle the vertex input data
struct VertexInputData {
	VkVertexInputBindingDescription vertexInputBinding;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
};

//Empty struct to allow for polymophism when passing vertex data to a mesh. 
// There is no pure virtual function for getting vertex input data because that adds extra data to the struct
// which messes up the passing of the data to shaders as Uniform Objects (such as when creating an SSBO and 
// sending it to a compute shader). It would also just adds needless extra data to each instance of the data
//struct VertexData {};

//Struct used for 3D vertex data for mesh (compatable with loading models)
struct Vertex3D {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoords;

	static VertexInputData getVertexInputData()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkVertexInputAttributeDescription description{};

		description.binding = 0;
		description.location = 0;
		description.format = VK_FORMAT_R32G32B32_SFLOAT;
		description.offset = offsetof(Vertex3D, position);

		attributeDescriptions.push_back(description);

		description.binding = 0;
		description.location = 1;
		description.format = VK_FORMAT_R32G32B32_SFLOAT;
		description.offset = offsetof(Vertex3D, color);

		attributeDescriptions.push_back(description);

		description.binding = 0;
		description.location = 2;
		description.format = VK_FORMAT_R32G32_SFLOAT;
		description.offset = offsetof(Vertex3D, texCoords);

		attributeDescriptions.push_back(description);

		VertexInputData vertexInputData{};
		vertexInputData.vertexInputBinding = bindingDescription;
		vertexInputData.vertexInputAttributes = attributeDescriptions;

		return vertexInputData;
	}

	bool operator==(const Vertex3D& other) const
	{
		return position == other.position && color == other.color && texCoords == other.texCoords;
	}
};

namespace std {
	template<> struct hash<Vertex3D> {
		size_t operator()(Vertex3D const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoords) << 1);
		}
	};
}

//Uniform object data (with corrected alignment) for particle 2D to be passed to shaders (in SSBOs)
struct Particle2D {
	glm::vec2 position;
	glm::vec2 velocity;
	alignas(16) glm::vec3 color; //Need to correct for alignment when passing to shaders

	static VertexInputData getParticleInputData()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Particle2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		VkVertexInputAttributeDescription description{};
		description.binding = 0;
		description.location = 0;
		description.format = VK_FORMAT_R32G32_SFLOAT;
		description.offset = offsetof(Particle2D, position);

		attributeDescriptions.push_back(description);

		description.binding = 0;
		description.location = 1;
		description.format = VK_FORMAT_R32G32B32_SFLOAT;
		description.offset = offsetof(Particle2D, color);

		attributeDescriptions.push_back(description);

		VertexInputData vertexInputData{};
		vertexInputData.vertexInputBinding = bindingDescription;
		vertexInputData.vertexInputAttributes = attributeDescriptions;

		return vertexInputData;
	}
};