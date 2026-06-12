#include "UniformBufferData.h"
#include "Helpers/BufferHelpers.h"
#include <random>

bool UniformBufferData::createDescriptorSetLayout(VkDevice logicalDevice)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = { uboLayoutBinding, samplerLayoutBinding };

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
	VkDeviceSize bufferSizeMVP = sizeof(ModelViewProjectionUniformObject);
	VkDeviceSize bufferSizeDT = sizeof(DeltaTimeUniformObject);

	mUniformBuffersMVP.resize(maxFramesBeingProcessed);
	mUniformBuffersMVPMemory.resize(maxFramesBeingProcessed);
	mUniformBuffersMVPMapped.resize(maxFramesBeingProcessed);

	mUniformBuffersDT.resize(maxFramesBeingProcessed);
	mUniformBuffersDTMemory.resize(maxFramesBeingProcessed);
	mUniformBuffersDTMapped.resize(maxFramesBeingProcessed);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		createBuffer(logicalDevice, physicalDevice, bufferSizeMVP, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffersMVP[i], mUniformBuffersMVPMemory[i]);

		if (vkMapMemory(logicalDevice, mUniformBuffersMVPMemory[i], 0, bufferSizeMVP, 0, &mUniformBuffersMVPMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}

		createBuffer(logicalDevice, physicalDevice, bufferSizeDT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffersDT[i], mUniformBuffersDTMemory[i]);

		if (vkMapMemory(logicalDevice, mUniformBuffersDTMemory[i], 0, bufferSizeDT, 0, &mUniformBuffersDTMapped[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to map memory for uniform buffer..." << std::endl;
			return false;
		}
	}

	return true;
}

bool UniformBufferData::createDescriptorPool(VkDevice logicalDevice, int maxFramesBeingProcessed)
{
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesBeingProcessed) * 2; // *2 for mvp and dt uniform buffers
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(maxFramesBeingProcessed);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(maxFramesBeingProcessed) * 2; // *2 for both prev and curr SSBOs

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(maxFramesBeingProcessed) * 2; // *2 for descriptor sets from graphics and compute pipelines

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create descriptor pool..." << std::endl;
		return false;
	}

	return true;
}

bool UniformBufferData::createDescriptorSets(VkDevice logicalDevice, VkImageView imageView, VkSampler sampler, int maxFramesBeingProcessed)
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
		std::cout << "\nFailed to allocate descriptor sets for graphics..." << std::endl;
		return false;
	}

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mUniformBuffersMVP[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ModelViewProjectionUniformObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool UniformBufferData::createComputeDescriptorSetLayout(VkDevice logicalDevice)
{
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].pImmutableSamplers = nullptr;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].pImmutableSamplers = nullptr;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].pImmutableSamplers = nullptr;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

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

bool UniformBufferData::createComputeDescriptorSets(VkDevice logicalDevice, int maxFramesBeingProcessed, int numParticles)
{ 
	std::vector<VkDescriptorSetLayout> layouts(maxFramesBeingProcessed, mComputeDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo{};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = mDescriptorPool;
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
		VkDescriptorBufferInfo dtBuffInfo{};
		dtBuffInfo.buffer = mUniformBuffersDT[i];
		dtBuffInfo.offset = 0;
		dtBuffInfo.range = sizeof(DeltaTimeUniformObject);

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mComputeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &dtBuffInfo;

		VkDescriptorBufferInfo ssboPrevFrame{};
		ssboPrevFrame.buffer = mSSBOs[(i - 1) % maxFramesBeingProcessed];
		ssboPrevFrame.offset = 0;
		ssboPrevFrame.range = sizeof(Particle2D) * numParticles;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mComputeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &ssboPrevFrame;

		VkDescriptorBufferInfo ssboCurrFrame{};
		ssboCurrFrame.buffer = mSSBOs[i];
		ssboCurrFrame.offset = 0;
		ssboCurrFrame.range = sizeof(Particle2D) * numParticles;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mComputeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &ssboCurrFrame;

		vkUpdateDescriptorSets(logicalDevice, 3, descriptorWrites.data(), 0, nullptr);
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

	VkDeviceSize bufferSize = sizeof(Particle2D) * numParticles;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory))
		{ return false; }

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, particles.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	for (int i = 0; i < maxFramesBeingProcessed; i++)
	{
		if (!createBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			mSSBOs[i], mSSBOsMemory[i]))
			{ return false; }

		copyBuffer(logicalDevice, stagingBuffer, mSSBOs[i], bufferSize, commandPool, submitQueue);
	}

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

	return true;
}

void UniformBufferData::updateUniformBuffer(int currFrame, float aspectRatio, float dt)
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

	memcpy(mUniformBuffersMVPMapped[currFrame], &mvpUniformObject, sizeof(mvpUniformObject));

	DeltaTimeUniformObject dtUniformObject{};
	dtUniformObject.dt = dt;

	memcpy(mUniformBuffersDTMapped[currFrame], &dtUniformObject, sizeof(dtUniformObject));
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

void UniformBufferData::cleanup(VkDevice logicalDevice)
{
	int numBuffers = mUniformBuffersMVP.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mUniformBuffersMVP[i], nullptr);
		vkFreeMemory(logicalDevice, mUniformBuffersMVPMemory[i], nullptr);
	}

	numBuffers = mUniformBuffersDT.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mUniformBuffersDT[i], nullptr);
		vkFreeMemory(logicalDevice, mUniformBuffersDTMemory[i], nullptr);
	}

	numBuffers = mSSBOs.size();

	for (int i = 0; i < numBuffers; i++)
	{
		vkDestroyBuffer(logicalDevice, mSSBOs[i], nullptr);
		vkFreeMemory(logicalDevice, mSSBOsMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, mComputeDescriptorSetLayout, nullptr);
}
