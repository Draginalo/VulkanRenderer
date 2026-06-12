#pragma once

#include "vulkan/vulkan.h"

#include "PipelineData.h"
#include "Mesh/Mesh3D.h"

//Needs a pointer to a pipeline to render with
// a uniform buffer for transform data
// a mesh pointer for the mesh to draw
class GameObject {
public:
	

private:
	PipelineData* mPipelineData;
	Mesh3D* meshToRender;
};