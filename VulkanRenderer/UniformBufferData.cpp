#include "UniformBufferData.h"
#include "Helpers/BufferHelpers.h"

bool UniformBufferData::createDescriptorSetLayout(VkDevice logicalDevice)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return true;
}

bool UniformBufferData::createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed)
{
	VkDeviceSize bufferSize = sizeof(ModelViewProjectionUniformObject);

	mUniformBuffers.resize(maxFramesBeingProcessed);
	mUniformBuffersMemory.resize(maxFramesBeingProcessed);
	mUniformBuffersMapped.resize(maxFramesBeingProcessed);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i], mUniformBuffersMemory[i]);

		if (vkMapMemory(logicalDevice, mUniformBuffersMemory[i], 0, bufferSize, 0, &mUniformBuffersMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}
	}

	return false;
}

bool UniformBufferData::createDescriptorPool(VkDevice logicalDevice, int maxFramesBeingProcessed)
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(maxFramesBeingProcessed);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(maxFramesBeingProcessed);

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create descriptor pool..." << std::endl;
		return false;
	}

	return true;
}

bool UniformBufferData::createDescriptorSets(VkDevice logicalDevice, int maxFramesBeingProcessed)
{
	std::vector<VkDescriptorSetLayout> layouts(maxFramesBeingProcessed, mDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo{};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = mDescriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesBeingProcessed);
	setAllocateInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize(maxFramesBeingProcessed);

	if (vkAllocateDescriptorSets(logicalDevice, &setAllocateInfo, mDescriptorSets.data()) != VK_SUCCESS)
	{
		return false;
	}

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ModelViewProjectionUniformObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}

	return true;
}

void UniformBufferData::updateUniformBuffer(int currFrame, float aspectRatio)
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

	memcpy(mUniformBuffersMapped[currFrame], &mvpUniformObject, sizeof(mvpUniformObject));
}

void UniformBufferData::bindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSets[currFrame], 
		0, nullptr);
}

void UniformBufferData::cleanup(VkDevice logicalDevice)
{
	int numBuffers = mUniformBuffers.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mUniformBuffers[i], nullptr);
		vkFreeMemory(logicalDevice, mUniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr);
}
