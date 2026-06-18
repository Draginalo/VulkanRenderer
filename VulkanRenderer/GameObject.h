#pragma once

#include "vulkan/vulkan.h"
#include <glm/glm.hpp>

#include "Pipelines//GraphicsPipeline.h"
#include "Mesh/Mesh3D.h"
#include "UniformDescriptorManager.h"

struct Transform {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale = {1.0, 1.0, 1.0};
};

//Needs a pointer to a pipeline to render with
// a uniform buffer for transform data
// a mesh pointer for the mesh to draw
class GameObject {
public:
	

private:
	Material* mpMaterial;
	Mesh3D* meshToRender;

	Transform mTranform;
	UniformDescriptorManager mUniformDescriptorManager;
};