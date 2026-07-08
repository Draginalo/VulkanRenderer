#include "VertexInputData.hpp"

VertexInputData Vertex3D::getVertexInputData() {
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

bool Vertex3D::operator==(const Vertex3D& other) const
{
	return position == other.position && color == other.color && texCoords == other.texCoords;
}

VertexInputData Particle2D::getParticleInputData()
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

std::size_t std::hash<Vertex3D>::operator()(const Vertex3D& vertex) const
{
	return ((std::hash<glm::vec3>()(vertex.position) ^
		(std::hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
		(std::hash<glm::vec2>()(vertex.texCoords) << 1);
}