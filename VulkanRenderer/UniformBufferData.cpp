#include "UniformBufferData.h"
#include "Helpers/BufferHelpers.h"
#include <random>

//Makse another class to return VkDescriptorSetLayoutBinding and VkWriteDescriptorSet with a class for each type to define 
// those which are passed to the "another" class which then prepares for passing to this class
bool UniformBufferData::createDescriptorSetLayout(VkDevice logicalDevice)
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = mDescriptorSetData.getLayoutBindings();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return true;
}

bool UniformBufferData::createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed)
{
	if (mDescriptorSetData.getTotalCombinedUniformBufferSize() == 0) { throw std::runtime_error("Need to add uniform data first..."); }

	mUniformBuffers.resize(maxFramesBeingProcessed);
	mUniformBuffersMemory.resize(maxFramesBeingProcessed);
	mUniformBuffersMapped.resize(maxFramesBeingProcessed);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		createBuffer(logicalDevice, physicalDevice, mDescriptorSetData.getTotalCombinedUniformBufferSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i],
			mUniformBuffersMemory[i]);

		if (vkMapMemory(logicalDevice, mUniformBuffersMemory[i], 0, mDescriptorSetData.getTotalCombinedUniformBufferSize(), 0,
			&mUniformBuffersMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}
	}

	return true;
}

bool UniformBufferData::createDescriptorSetsData(VkDevice logicalDevice, VkDescriptorPool descriptorPool, int maxFramesBeingProcessed)
{
	std::vector<VkDescriptorSetLayout> layouts(maxFramesBeingProcessed, mDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo{};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = descriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesBeingProcessed);
	setAllocateInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize(maxFramesBeingProcessed);

	if (vkAllocateDescriptorSets(logicalDevice, &setAllocateInfo, mDescriptorSets.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate descriptor sets for graphics..." << std::endl;
		return false;
	}

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites = 
			mDescriptorSetData.getWriteDescriptorSets(mDescriptorSets[i], mUniformBuffers, mSSBOs, i);

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool UniformBufferData::createComputeDescriptorSetLayout(VkDevice logicalDevice)
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = mDescriptorSetData.getComputeLayoutBindings();

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &mComputeDescriptorSetLayout) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create descriptor set layout for compute..." << std::endl;
		return false;
	}

	return true;
}

bool UniformBufferData::createComputeDescriptorSets(VkDevice logicalDevice, VkDescriptorPool descriptorPool, 
	int maxFramesBeingProcessed, int numParticles)
{ 
	std::vector<VkDescriptorSetLayout> layouts(maxFramesBeingProcessed, mComputeDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo{};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = descriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesBeingProcessed);
	setAllocateInfo.pSetLayouts = layouts.data();

	mComputeDescriptorSets.resize(maxFramesBeingProcessed);

	if (vkAllocateDescriptorSets(logicalDevice, &setAllocateInfo, mComputeDescriptorSets.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate descriptor sets for compute..." << std::endl;
		return false;
	}

	for (size_t i = 0; i < maxFramesBeingProcessed; i++)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites = mDescriptorSetData.getComputeWriteDescriptorSets(mComputeDescriptorSets[i], 
			mUniformBuffers, mSSBOs, i);

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool UniformBufferData::createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed,
	const int numParticles, float recipAspect, VkCommandPool commandPool, VkQueue submitQueue)
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

	VkDeviceSize bufferSize = mDescriptorSetData.getTotalCombinedStorageBufferSize();

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		//TODO: Add a way to set the flags like: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			mSSBOs[i], mSSBOsMemory[i]))
			{ return false; }
	}

	addDataToSSBOs(logicalDevice, physicalDevice, particles.data(), mDescriptorSetData.getComputeUniformBufferDescriptors()[2]);

	return true;
}

void UniformBufferData::updateUniformBuffers(int currFrame, float aspectRatio, float dt)
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

	mDescriptorSetData.getUniformBufferDescriptors()[0]->setDataPointer(&mvpUniformObject);

	DeltaTimeUniformObject dtUniformObject{};
	dtUniformObject.dt = dt;

	mDescriptorSetData.getComputeUniformBufferDescriptors()[0]->setDataPointer(&dtUniformObject);

	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObjectData().memPointer = &dtUniformObject;
	//mDescriptorSetData.getUniformDescriptors()[1]->getUniformObssjectData().size = sizeof(dtUniformObject);

	mDescriptorSetData.updateBufferUniforms(mUniformBuffersMapped[currFrame]);
	mDescriptorSetData.updateComputeBufferUniforms(mUniformBuffersMapped[currFrame]);
}

void UniformBufferData::bindGraphicsDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSets[currFrame], 
		0, nullptr);
}

void UniformBufferData::bindComputeDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &mComputeDescriptorSets[currFrame],
		0, nullptr);
}

void UniformBufferData::bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mSSBOs[currFrame], offsets);

	vkCmdDraw(commandBuffer, numParticles, 1, 0, 0);
}

bool UniformBufferData::addDataToSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, void* pData, 
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

void UniformBufferData::cleanup(VkDevice logicalDevice)
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

	vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, mComputeDescriptorSetLayout, nullptr);
}
