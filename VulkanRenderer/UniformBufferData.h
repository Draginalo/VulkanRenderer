#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan.h"
#include <iostream>
#include <vector>
#include <chrono>

struct ModelViewProjectionUniformObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class UniformBufferData {
public:
	bool createDescriptorSetLayout(VkDevice logicalDevice);
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorPool(VkDevice logicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorSets(VkDevice logicalDevice, int maxFramesBeingProcessed);

	void updateUniformBuffer(int currFrame, float aspectRatio);
	void bindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame);

	void cleanup(VkDevice logicalDevice);
	inline VkDescriptorSetLayout getDescriptorSetLayout() { return mDescriptorSetLayout; }
private:
	VkDescriptorSetLayout mDescriptorSetLayout;

	std::vector<VkBuffer> mUniformBuffers;
	std::vector<VkDeviceMemory> mUniformBuffersMemory;
	std::vector<void*> mUniformBuffersMapped;

	VkDescriptorPool mDescriptorPool;
	std::vector<VkDescriptorSet> mDescriptorSets;
};