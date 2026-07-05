#pragma once

#include "vulkan/vulkan.h"
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//Helper struct to bundle the vertex input data
struct VertexInputData {
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	VkVertexInputBindingDescription vertexInputBinding;

	//Padding for compiler warning
	uint8_t padding[4] = {};
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

	static VertexInputData getVertexInputData();

	bool operator==(const Vertex3D& other) const;
};

template<> struct std::hash<Vertex3D> {
	std::size_t operator()(const Vertex3D& vertex) const;
};

//Uniform object data (with corrected alignment) for particle 2D to be passed to shaders (in SSBOs)
struct Particle2D {
	glm::vec2 position = {};
	glm::vec2 velocity = {};
	glm::vec3 color = {}; //Need to correct for alignment when passing to shaders

	//Padding for compiler warning
	uint8_t padding[4] = {};

	static VertexInputData getParticleInputData();
};
