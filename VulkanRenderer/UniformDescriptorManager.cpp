#include "UniformDescriptorManager.h"
#include "Helpers/BufferHelpers.h"

void UniformDescriptorManager::createPipelineSpecificDescriptorSets(std::vector<Pipeline*> pipelines, VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice, VkDescriptorPool descriptorPool, int maxFramesBeingProcessed)
{
	int numSets = pipelines.size();
	VkDeviceSize totalUniformBuffersSize = 0;
	VkDeviceSize totalStorageBuffersSize = 0;

	for (int i = 0; i < numSets; i++)
	{
		//Check if already added pipeline descriptor set data
		if (mPipelineSpecificDescriptorSets.find(pipelines[i]->getPipelineID()) != mPipelineSpecificDescriptorSets.end()) { continue; }

		mPipelineSpecificDescriptorSets[pipelines[i]->getPipelineID()] = pipelines[i]->getPipelineDescriptorSetData();
		pipelines[i]->createPipelineDescriptorSetLayout(logicalDevice);

		totalUniformBuffersSize += pipelines[i]->getPipelineDescriptorSetData()->getTotalUniformBufferSize();
		totalStorageBuffersSize += pipelines[i]->getPipelineDescriptorSetData()->getTotalStorageBufferSize();
	}

	createUniformBuffers(logicalDevice, physicalDevice, totalUniformBuffersSize, maxFramesBeingProcessed);
	createSSBOs(logicalDevice, physicalDevice, totalStorageBuffersSize, maxFramesBeingProcessed);

	BufferData uniformBufferData{};
	BufferData storageBufferData{};

	uniformBufferData.buffers = mUniformBuffers;
	storageBufferData.buffers = mSSBOs;

	for (int i = 0; i < numSets; i++)
	{
		pipelines[i]->createPipelineDescriptorSetData(logicalDevice, descriptorPool, &uniformBufferData, &storageBufferData, 
			maxFramesBeingProcessed);
	}
}

bool UniformDescriptorManager::createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, 
	int maxFramesBeingProcessed)
{
	mUniformBuffers.resize(maxFramesBeingProcessed);
	mUniformBuffersMemory.resize(maxFramesBeingProcessed);
	mUniformBuffersMapped.resize(maxFramesBeingProcessed);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i],
			mUniformBuffersMemory[i]);

		if (vkMapMemory(logicalDevice, mUniformBuffersMemory[i], 0, bufferSize, 0, &mUniformBuffersMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}
	}

	return true;
}

bool UniformDescriptorManager::createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, 
	int maxFramesBeingProcessed)
{
	mSSBOs.resize(maxFramesBeingProcessed);
	mSSBOsMemory.resize(maxFramesBeingProcessed);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		//TODO: Add a way to set the flags like: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			mSSBOs[i], mSSBOsMemory[i]))
			{ return false; }
	}

	return true;
}

void UniformDescriptorManager::updatePipelineSpecificUniformBuffers(Pipeline* graphicsPipeline, Pipeline* computePipeline, 
	int currFrame, float aspectRatio, float dt)
{
	static std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ModelViewProjectionUniformObject mvpUniformObject{};
	mvpUniformObject.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	mvpUniformObject.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	mvpUniformObject.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);

	//Flips y scaling factor since glm presumes inverted y coord
	mvpUniformObject.proj[1][1] *= -1;

	graphicsPipeline->getPipelineDescriptorSetDataRef()->getUniformBufferDescriptors()[0]->setDataPointer(&mvpUniformObject);

	DeltaTimeUniformObject dtUniformObject{};
	dtUniformObject.dt = dt;

	computePipeline->getPipelineDescriptorSetDataRef()->getUniformBufferDescriptors()[0]->setDataPointer(&dtUniformObject);

	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObjectData().memPointer = &dtUniformObject;
	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObssjectData().size = sizeof(dtUniformObject);

	for (const std::pair<const uint32_t, const DescriptorSetData*>& keyValuePair : mPipelineSpecificDescriptorSets)
	{
		keyValuePair.second->updateBufferUniforms(mUniformBuffersMapped[currFrame]);
	}
}

void UniformDescriptorManager::bindPipelineSpecificDescriptorSet(VkCommandBuffer commandBuffer, Pipeline* pipeline, 
	int currFrame)
{
	const DescriptorSetData* pipelineDescriptorSetData = mPipelineSpecificDescriptorSets[pipeline->getPipelineID()];

	VkPipelineBindPoint bindPoint = pipeline->getIsComputePipeline() ? 
		VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipeline->getPipelineLayout(), 0, 1,
		pipelineDescriptorSetData->getDescriptorSet(currFrame), 0, nullptr);
}

void UniformDescriptorManager::bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mSSBOs[currFrame], offsets);

	vkCmdDraw(commandBuffer, numParticles, 1, 0, 0);
}

bool UniformDescriptorManager::addDataToSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, void* pData, 
	UniformBufferDescriptor* pDescriptorData)
{
	if (pDescriptorData->getSourceFromPastFrame()) { 
		std::cout << "\nCan't add data to previous frame data" << std::endl;
		return false; 
	}

	size_t numBuffers = mSSBOsMemory.size();

	for (int i = 0; i < numBuffers; i++)
	{
		void* bufferDataPointer;
		vkMapMemory(logicalDevice, mSSBOsMemory[i], pDescriptorData->getOffset(), pDescriptorData->getDataSize(), 0, &bufferDataPointer);
		memcpy(bufferDataPointer, pData, (size_t)pDescriptorData->getDataSize());
		vkUnmapMemory(logicalDevice, mSSBOsMemory[i]);
	}

	return true;
}

void UniformDescriptorManager::cleanup(VkDevice logicalDevice)
{
	int numBuffers = mUniformBuffers.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mUniformBuffers[i], nullptr);
		vkFreeMemory(logicalDevice, mUniformBuffersMemory[i], nullptr);
	}

	numBuffers = mSSBOs.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mSSBOs[i], nullptr);
		vkFreeMemory(logicalDevice, mSSBOsMemory[i], nullptr);
	}

	mPipelineSpecificDescriptorSets.clear();
}
