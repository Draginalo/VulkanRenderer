#include "Mesh3D.hpp"
#include "VulkanRenderer/Helpers/ModelHelpers.hpp"

bool Mesh3D::createVertexDataFromModel(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
	VkQueue submitQueue, const char* modelFilepath)
{
	std::vector<Vertex3D> vertices;

	if (!loadModel(modelFilepath, &vertices, &mIndecies)) { return false; }

	VkDeviceSize size = sizeof(Vertex3D) * vertices.size();
	createVertexBuffer(logicalDevice, physicalDevice, commandPool, submitQueue, vertices.data(), size,
		mIndecies, Vertex3D::getVertexInputData());

	createIndeciesBuffer(logicalDevice, physicalDevice, commandPool, submitQueue);

	return true;
}
