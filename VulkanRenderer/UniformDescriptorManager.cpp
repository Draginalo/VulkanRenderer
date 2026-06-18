#include "UniformDescriptorManager.h"
#include "Helpers/BufferHelpers.h"

void UniformDescriptorManager::createPipelineSpecificDescriptorSets(std::vector<Pipeline*> pipelines, VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice, int maxFramesInFlight)
{
	int numSets = pipelines.size();
	VkDeviceSize totalUniformBuffersSize = 0;
	VkDeviceSize totalStorageBuffersSize = 0;

	for (int i = 0; i < numSets; i++)
	{
		pipelines[i]->createPipelineDescriptorSetLayout(logicalDevice);
		pipelines[i]->createBaseMaterialDescriptorSetLayout(logicalDevice);

		totalUniformBuffersSize += pipelines[i]->getPipelineDescriptorSetData()->getTotalUniformBufferSize();
		totalStorageBuffersSize += pipelines[i]->getPipelineDescriptorSetData()->getTotalStorageBufferSize();
	}

	createUniformBuffers(logicalDevice, physicalDevice, totalUniformBuffersSize, maxFramesInFlight);
	createSSBOs(logicalDevice, physicalDevice, totalStorageBuffersSize, maxFramesInFlight);

	BufferData uniformBufferData{};
	BufferData storageBufferData{};

	uniformBufferData.buffers = mPipelineUniformBuffers;
	storageBufferData.buffers = mPipelineDescriptorSSBOs;

	for (int i = 0; i < numSets; i++)
	{
		pipelines[i]->createPipelineDescriptorSetData(logicalDevice, mDescriptorPool, &uniformBufferData, &storageBufferData, 
			maxFramesInFlight);
		pipelines[i]->createBaseMaterialDescriptorSetData(logicalDevice, mDescriptorPool, &uniformBufferData, &storageBufferData,
			maxFramesInFlight);
	}
}

void UniformDescriptorManager::createMaterialSpecificDescriptorSets(std::vector<Material*> materials, VkDevice logicalDevice, 
	VkPhysicalDevice physicalDevice, int maxFramesInFlight)
{
}

bool UniformDescriptorManager::createDescriptorPool(VkDevice logicalDevice, std::vector<Pipeline*> pipelines, int maxFramesInFlight)
{
	std::vector<VkDescriptorPoolSize> poolSizes{};

	std::unordered_map<VkDescriptorType, uint32_t> bufferTypeCountTracker;

	uint32_t maxSets = 0;

	for (const Pipeline* pipeline : pipelines)
	{
		//Accounts for each descriptor set per per pipeline
		maxSets++;

		for (const UniformBufferDescriptor& buffDescriptor : *pipeline->getPipelineDescriptorSetData()->getUniformBufferDescriptors())
		{
			if (bufferTypeCountTracker.find(buffDescriptor.getUniformType()) == bufferTypeCountTracker.end())
			{
				bufferTypeCountTracker[buffDescriptor.getUniformType()] = 1;
			}
			else {
				bufferTypeCountTracker[buffDescriptor.getUniformType()]++;
			}
		}

		for (const UniformImageDescriptor& imageDescriptor : *pipeline->getPipelineDescriptorSetData()->getUniformImageDescriptors())
		{
			if (bufferTypeCountTracker.find(imageDescriptor.getUniformType()) == bufferTypeCountTracker.end())
			{
				bufferTypeCountTracker[imageDescriptor.getUniformType()] = 1;
			}
			else {
				bufferTypeCountTracker[imageDescriptor.getUniformType()]++;
			}
		}

		for (const UniformBufferDescriptor& buffDescriptor : 
			*pipeline->getBaseMaterial()->materialDescriptorSetData.getUniformBufferDescriptors())
		{
			if (bufferTypeCountTracker.find(buffDescriptor.getUniformType()) == bufferTypeCountTracker.end())
			{
				bufferTypeCountTracker[buffDescriptor.getUniformType()] = 1;
			}
			else {
				bufferTypeCountTracker[buffDescriptor.getUniformType()]++;
			}
		}

		for (const UniformImageDescriptor& imageDescriptor : 
			*pipeline->getBaseMaterial()->materialDescriptorSetData.getUniformImageDescriptors())
		{
			if (bufferTypeCountTracker.find(imageDescriptor.getUniformType()) == bufferTypeCountTracker.end())
			{
				bufferTypeCountTracker[imageDescriptor.getUniformType()] = 1;
			}
			else {
				bufferTypeCountTracker[imageDescriptor.getUniformType()]++;
			}
		}

		//Accounts for all materials of a pipeline (including the base material set with the + 1)
		maxSets += pipeline->getPipelineMaterials()->size() + 1;
	}

	for (const std::pair<VkDescriptorType, uint32_t>& descriptorCount : bufferTypeCountTracker)
	{
		poolSizes.push_back({ descriptorCount.first, descriptorCount.second * maxFramesInFlight });
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets * maxFramesInFlight; 

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create descriptor pool..." << std::endl;
		return false;
	}

	return true;
}

bool UniformDescriptorManager::createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, 
	int maxFramesInFlight)
{
	mPipelineUniformBuffers.resize(maxFramesInFlight);
	mPipelineUniformBuffersMemory.resize(maxFramesInFlight);
	mPipelineUniformBuffersMapped.resize(maxFramesInFlight);

	for (int i = 0; i < maxFramesInFlight; i++)
	{
		createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mPipelineUniformBuffers[i],
			mPipelineUniformBuffersMemory[i]);

		if (vkMapMemory(logicalDevice, mPipelineUniformBuffersMemory[i], 0, bufferSize, 0, &mPipelineUniformBuffersMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}
	}

	return true;
}

bool UniformDescriptorManager::createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, 
	int maxFramesInFlight)
{
	mPipelineDescriptorSSBOs.resize(maxFramesInFlight);
	mPipelineDescriptorSSBOsMemory.resize(maxFramesInFlight);

	for (int i = 0; i < maxFramesInFlight; i++)
	{
		//TODO: Add a way to set the flags like: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			mPipelineDescriptorSSBOs[i], mPipelineDescriptorSSBOsMemory[i]))
			{ return false; }
	}

	return true;
}

void UniformDescriptorManager::updatePipelineSpecificUniformBuffers(std::vector<Pipeline*> activePipelines, int currFrame, 
	float aspectRatio, float dt)
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

	activePipelines[0]->getPipelineBufferDescriptorRef(0)->setDataPointer(&mvpUniformObject);

	DeltaTimeUniformObject dtUniformObject{};
	dtUniformObject.dt = dt;

	activePipelines[1]->getPipelineBufferDescriptorRef(0)->setDataPointer(&dtUniformObject);

	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObjectData().memPointer = &dtUniformObject;
	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObssjectData().size = sizeof(dtUniformObject);

	for (const Pipeline* pipeline : activePipelines)
	{
		pipeline->getPipelineDescriptorSetData()->updateBufferUniforms(mPipelineUniformBuffersMapped[currFrame]);
	}
}

void UniformDescriptorManager::bindPipelineSpecificDescriptorSet(VkCommandBuffer commandBuffer, Pipeline* pipeline, 
	int currFrame)
{
	VkPipelineBindPoint bindPoint = pipeline->getIsComputePipeline() ? 
		VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipeline->getPipelineLayout(), 0, 1,
		pipeline->getPipelineDescriptorSetData()->getDescriptorSet(currFrame), 0, nullptr);
}

void UniformDescriptorManager::bindMaterialSpecificDescriptorSet(VkCommandBuffer commandBuffer, const Material* material, int currFrame)
{
	VkPipelineBindPoint bindPoint = material->pipelineForMaterial->getIsComputePipeline() ?
		VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

	vkCmdBindDescriptorSets(commandBuffer, bindPoint, material->pipelineForMaterial->getPipelineLayout(), 1, 1,
		material->materialDescriptorSetData.getDescriptorSet(currFrame), 0, nullptr);
}

void UniformDescriptorManager::bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mPipelineDescriptorSSBOs[currFrame], offsets);

	vkCmdDraw(commandBuffer, numParticles, 1, 0, 0);
}

bool UniformDescriptorManager::addDataToSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, void* pData, 
	const UniformBufferDescriptor* pDescriptorData)
{
	if (pDescriptorData->getSourceFromPastFrame()) { 
		std::cout << "\nCan't add data to previous frame data" << std::endl;
		return false; 
	}

	size_t numBuffers = mPipelineDescriptorSSBOsMemory.size();

	for (int i = 0; i < numBuffers; i++)
	{
		void* bufferDataPointer;
		vkMapMemory(logicalDevice, mPipelineDescriptorSSBOsMemory[i], pDescriptorData->getOffset(), pDescriptorData->getDataSize(), 0, &bufferDataPointer);
		memcpy(bufferDataPointer, pData, (size_t)pDescriptorData->getDataSize());
		vkUnmapMemory(logicalDevice, mPipelineDescriptorSSBOsMemory[i]);
	}

	return true;
}

void UniformDescriptorManager::cleanup(VkDevice logicalDevice)
{
	int numBuffers = mPipelineUniformBuffers.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mPipelineUniformBuffers[i], nullptr);
		vkFreeMemory(logicalDevice, mPipelineUniformBuffersMemory[i], nullptr);
	}

	numBuffers = mPipelineDescriptorSSBOs.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mPipelineDescriptorSSBOs[i], nullptr);
		vkFreeMemory(logicalDevice, mPipelineDescriptorSSBOsMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
}
