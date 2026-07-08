#pragma once

#include "MeshGeneric.hpp"
#include <vulkan/vulkan.h>

//TODO: Make this mesh class inherit from a base class (where the derived classes load data differently, ex. one 
// from model and the other a regular vector)
class Mesh3D : public MeshGeneric {
public:
	bool createVertexDataFromModel(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		VkQueue submitQueue, const char* modelFilepath);
private:
	
};