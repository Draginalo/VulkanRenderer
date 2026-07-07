#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Pipelines/GraphicsPipeline.h"
#include "Mesh/Mesh3D.h"
#include "UniformDescriptorHandlers/UniformDescriptorManager.h"

struct Transform {
	glm::vec3 position = { 0.0, 0.0, 0.0 };
	glm::vec3 rotation = { 0.0, 0.0, 0.0 };
	glm::vec3 scale = {1.0, 1.0, 1.0};
};

//Needs a pointer to a pipeline to render with
// a uniform buffer for transform data
// a mesh pointer for the mesh to draw
class GameObject {
public:
	GameObject();
	GameObject(const Material* pMaterial, const Mesh3D* pMeshToRender);

	const Material* getMaterial() const;
	const Mesh3D* getMesh() const;

private:
	//UniformDescriptorManager mUniformDescriptorManager;

	const Material* mpMaterial = nullptr;
	const Mesh3D* mpMeshToRender = nullptr;

	Transform mTranform;

	//Explicit padding for compiler warning
	uint8_t padding[4] = {0,0,0,0};
};