#include "RenderGraph.h"

void RenderGraph::handleAddPipelineToOrderedList(Pipeline* pipelineToAdd)
{
	int pipelineCount = mActivePipelines_Ordered.size();
	int lastPipelineToDependOn = -1;
	int lastPipelineThatDependsOn = -1;
	for (int i = 0; i < pipelineCount; i++)
	{
		//TODO: Add automatic detection of pipeline dependency here and setting of memory barrier for pipeline here
		if (pipelineToAdd->getPipelineDependencyInfo()->mDependsOnPipeline == mActivePipelines_Ordered[i])
		{
			lastPipelineToDependOn = i;
		}

		if (mActivePipelines_Ordered[i]->getPipelineDependencyInfo()->mDependsOnPipeline == pipelineToAdd)
		{
			lastPipelineThatDependsOn = i;
		}
	}

	if (lastPipelineThatDependsOn == -1)
	{
		mActivePipelines_Ordered.push_back(pipelineToAdd);
		return;
	}

	mActivePipelines_Ordered.insert(mActivePipelines_Ordered.begin() + lastPipelineThatDependsOn, pipelineToAdd);
}

void RenderGraph::addDrawableToRenderTree(DrawableData drawableDataToAdd)
{
	if (mRenderTree.find(drawableDataToAdd.pipelineToDrawWith) == mRenderTree.end())
	{
		mRenderTree[drawableDataToAdd.pipelineToDrawWith] = {};
		handleAddPipelineToOrderedList(drawableDataToAdd.pipelineToDrawWith);
	}

	if (mRenderTree[drawableDataToAdd.pipelineToDrawWith].find(drawableDataToAdd.materialToDrawWith) ==
		mRenderTree[drawableDataToAdd.pipelineToDrawWith].end())
	{
		mRenderTree[drawableDataToAdd.pipelineToDrawWith][drawableDataToAdd.materialToDrawWith] = {};
	}

	mRenderTree[drawableDataToAdd.pipelineToDrawWith][drawableDataToAdd.materialToDrawWith].push_back(
		drawableDataToAdd.drawable);
}

void RenderGraph::removeDrawableFromRenderTree(const DrawableData drawableDataToRemove)
{
	if (mRenderTree.find(drawableDataToRemove.pipelineToDrawWith) == mRenderTree.end())
	{
		return;
	}

	if (mRenderTree[drawableDataToRemove.pipelineToDrawWith].find(drawableDataToRemove.materialToDrawWith) ==
		mRenderTree[drawableDataToRemove.pipelineToDrawWith].end())
	{
		return;
	}

	//Finds and removes mesh from render tree
	std::vector<const Drawable*>* meshListForMat = &mRenderTree[drawableDataToRemove.pipelineToDrawWith][drawableDataToRemove.materialToDrawWith];
	std::vector<const Drawable*>::iterator element = std::find(meshListForMat->begin(), meshListForMat->end(), drawableDataToRemove.drawable);

	if (element == meshListForMat->end()) { return; }

	meshListForMat->erase(element);

	//Removes material from render graph if no more drawables use it
	if (meshListForMat->empty())
	{
		mRenderTree[drawableDataToRemove.pipelineToDrawWith].erase(drawableDataToRemove.materialToDrawWith);

		//Removes pipeline from render graph and active pipelines list if no more materials are present that use the pipeline
		if (mRenderTree[drawableDataToRemove.pipelineToDrawWith].empty())
		{
			mRenderTree.erase(drawableDataToRemove.pipelineToDrawWith);

			std::vector<Pipeline*>::iterator pipelineElement = std::find(mActivePipelines_Ordered.begin(),
				mActivePipelines_Ordered.end(), drawableDataToRemove.pipelineToDrawWith);
			mActivePipelines_Ordered.erase(pipelineElement);
		}
	}
}

void RenderGraph::buildRenderTree(std::vector<DrawableData> activeDrawablesData)
{
	for (const DrawableData& activeDrawableData : activeDrawablesData)
	{
		addDrawableToRenderTree(activeDrawableData);
	}
}