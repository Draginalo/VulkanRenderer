#pragma once

#include "Pipelines/Pipeline.h"
#include "Mesh/Drawable.h"
#include <unordered_map>
#include <vector>
#include "GameObject.h"

struct DrawableData {
	const Drawable* drawable;
	Pipeline* pipelineToDrawWith;
	const Material* materialToDrawWith = nullptr;
};

class RenderGraph {
public:
	void handleAddPipelineToOrderedList(Pipeline* pipelineToAdd);
	void addDrawableToRenderTree(DrawableData drawableDataToAdd);
	void removeDrawableFromRenderTree(const DrawableData drawableDataToRemove);
	void buildRenderTree(std::vector<DrawableData> activeDrawablesData);

	void handleRegisterDrawableDependencyData(DrawableData drawableData);
	void handleRegisterPipelineDependencyData(Pipeline* pipeline);

	std::vector<Pipeline*>* getOrderedActivePipelines();
	std::unordered_map<Pipeline*, std::unordered_map<const Material*, std::vector<const Drawable*>>>&
		getRenderTree();
private:
	std::vector<Pipeline*> mActivePipelines_Ordered;
	std::unordered_map<Pipeline*, std::unordered_map<const Material*, std::vector<const Drawable*>>> mRenderTree;
};