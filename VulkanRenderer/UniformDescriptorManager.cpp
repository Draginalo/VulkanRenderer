#include "UniformDescriptorManager.h"
#include "Helpers/BufferHelpers.h"
#include <random>

void UniformDescriptorManager::createPipelineSpecificDescriptorSets(std::vector<DescriptorSetData*> descriptorSets, VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice, VkDescriptorPool descriptorPool, int maxFramesBeingProcessed,
	const int numParticles, float recipAspect)
{

	mPipelineSpecificDescriptorSets = descriptorSets;

	int numSets = descriptorSets.size();
	VkDeviceSize totalUniformBuffersSize = 0;
	VkDeviceSize totalStorageBuffersSize = 0;

	for (int i = 0; i < numSets; i++)
	{
		descriptorSets[i]->createDescriptorSetLayout(logicalDevice);

		totalUniformBuffersSize += descriptorSets[i]->getTotalUniformBufferSize();
		totalStorageBuffersSize += descriptorSets[i]->getTotalStorageBufferSize();
	}

	createUniformBuffers(logicalDevice, physicalDevice, totalUniformBuffersSize, maxFramesBeingProcessed);
	createSSBOs(logicalDevice, physicalDevice, totalStorageBuffersSize, maxFramesBeingProcessed, numParticles, recipAspect);

	BufferData uniformBufferData{};
	BufferData storageBufferData{};

	uniformBufferData.buffers = mUniformBuffers;
	storageBufferData.buffers = mSSBOs;

	for (int i = 0; i < numSets; i++)
	{
		descriptorSets[i]->createDescriptorSetData(logicalDevice, descriptorPool, &uniformBufferData, &storageBufferData, 
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
	int maxFramesBeingProcessed, const int numParticles, float recipAspect)
{
	mSSBOs.resize(maxFramesBeingProcessed);
	mSSBOsMemory.resize(maxFramesBeingProcessed);

	std::vector<Particle2D> particles(numParticles);

	std::default_random_engine rngEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rngRange(0.0f, 1.0f);

	for (int i = 0; i < numParticles; i++)
	{
		float r = 0.25 * sqrt(rngRange(rngEngine));
		float theta = rngRange(rngEngine) * 2.0 * 3.14159f;
		float x = r * cos(theta) * recipAspect;
		float y = r * sin(theta);
		particles[i].position = glm::vec2(x, y);
		particles[i].velocity = glm::normalize(particles[i].position) * 0.00025f * (rngRange(rngEngine) + 0.3f);
		particles[i].color = glm::vec3(rngRange(rngEngine), rngRange(rngEngine), rngRange(rngEngine));
	}


	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		//TODO: Add a way to set the flags like: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			mSSBOs[i], mSSBOsMemory[i]))
			{ return false; }
	}

	addDataToSSBOs(logicalDevice, physicalDevice, particles.data(), mPipelineSpecificDescriptorSets[1]->getUniformBufferDescriptors()[2]);

	return true;
}

void UniformDescriptorManager::updatePipelineSpecificUniformBuffers(int currFrame, float aspectRatio, float dt)
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

	mPipelineSpecificDescriptorSets[0]->getUniformBufferDescriptors()[0]->setDataPointer(&mvpUniformObject);

	DeltaTimeUniformObject dtUniformObject{};
	dtUniformObject.dt = dt;

	mPipelineSpecificDescriptorSets[1]->getUniformBufferDescriptors()[0]->setDataPointer(&dtUniformObject);

	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObjectData().memPointer = &dtUniformObject;
	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObssjectData().size = sizeof(dtUniformObject);

	int numDescriptorSets = mPipelineSpecificDescriptorSets.size();

	for (int i = 0; i < numDescriptorSets; i++)
	{
		mPipelineSpecificDescriptorSets[i]->updateBufferUniforms(mUniformBuffersMapped[currFrame]);
	}
}

void UniformDescriptorManager::bindGraphicsDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame)
{

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, 
		mPipelineSpecificDescriptorSets[0]->getDescriptorSet(currFrame), 0, nullptr);
}

void UniformDescriptorManager::bindComputeDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, 
		mPipelineSpecificDescriptorSets[1]->getDescriptorSet(currFrame), 0, nullptr);
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

	int numDescriptorSets = mPipelineSpecificDescriptorSets.size();

	for (int i = 0; i < numDescriptorSets; i++)
	{
		mPipelineSpecificDescriptorSets[i]->cleanup(logicalDevice);
	}

	mPipelineSpecificDescriptorSets.clear();
}
