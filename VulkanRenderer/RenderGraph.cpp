#include "RenderGraph.h"

void RenderGraph::handleAddPipelineToOrderedList(Pipeline* pipelineToAdd)
{
	int pipelineCount = mActivePipelines_Ordered.size();
	int lastPipelineToDependOn = -1;
	int lastPipelineThatDependsOn = -1;

	handleRegisterPipelineDependencyData(pipelineToAdd);

	for (int i = 0; i < pipelineCount; i++)
	{
		//TODO: Add automatic detection of pipeline dependency here and setting of memory barrier for pipeline here
		if (pipelineToAdd->getPipelineDependencyInfo()->dependsOnPipeline == mActivePipelines_Ordered[i])
		{
			lastPipelineToDependOn = i;
		}

		if (mActivePipelines_Ordered[i]->getPipelineDependencyInfo()->dependsOnPipeline == pipelineToAdd)
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

	handleRegisterDrawableDependencyData(drawableDataToAdd);
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

void RenderGraph::handleRegisterDrawableDependencyData(DrawableData drawableData)
{
	//Handles checking if drawable being added has a dependencies from another pipeline (such as the drawable rendering a buffer being 
	// written to by another active pipeline)
	if (drawableData.drawable == nullptr) { return; }

	Pipeline* pipelineToDependOn = drawableData.pipelineToDrawWith->getPipelineDependencyInfo()->dependsOnPipeline;

	if (pipelineToDependOn == nullptr) { return; }

	//Forms memory barrier data for the buffers that are outputs on the pipeline marked as depending on and are inputs for drawing 
	// the current drawable
	PipelineDependencyInfo* depInfo = drawableData.pipelineToDrawWith->getPipelineDependencyInfo();
	for (const UniformBufferDescriptor& buffDescriptor : *pipelineToDependOn->getPipelineDescriptorSetData()->getUniformBufferDescriptors())
	{
		if (buffDescriptor.getCurrBuffer() == drawableData.drawable->getLastFrameVertexBuffer())
		{
			VkBufferMemoryBarrier2 buffBarrier{};
			buffBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			buffBarrier.buffer = buffDescriptor.getCurrBuffer();
			buffBarrier.size = buffDescriptor.getDataSize();
			buffBarrier.offset = buffDescriptor.getOffset();

			//Adds memory barrier data to pipeline (TODO: Since this memory barrier data is based on a specific drawable and not a 
			// whole pipeline, perhaps look into injecting memory barriers per object)
			depInfo->buffMemBarriers.push_back(buffBarrier);
		}
	}
}

void RenderGraph::handleRegisterPipelineDependencyData(Pipeline* pipeline)
{
	Pipeline* pipelineToDependOn = pipeline->getPipelineDependencyInfo()->dependsOnPipeline;

	if (pipelineToDependOn == nullptr) { return; }

	//Forms memory barrier data for the buffers that are outputs on the pipeline marked as depending on and are inputs for  
	// the current pipeline
	PipelineDependencyInfo* depInfo = pipeline->getPipelineDependencyInfo();
	for (const UniformBufferDescriptor& buffDescriptor : *pipelineToDependOn->getPipelineDescriptorSetData()->getUniformBufferDescriptors())
	{
		for (const UniformBufferDescriptor& otherBuffDescriptor : *pipeline->getPipelineDescriptorSetData()->getUniformBufferDescriptors())
		{
			if (buffDescriptor.getCurrBuffer() == otherBuffDescriptor.getCurrBuffer())
			{
				VkBufferMemoryBarrier2 buffBarrier{};
				buffBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				buffBarrier.buffer = buffDescriptor.getCurrBuffer();
				buffBarrier.size = buffDescriptor.getDataSize();
				buffBarrier.offset = buffDescriptor.getOffset();

				//Adds memory barrier
				depInfo->buffMemBarriers.push_back(buffBarrier);
			}
		}
	}
}
