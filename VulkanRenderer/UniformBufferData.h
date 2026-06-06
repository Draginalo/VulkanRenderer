#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan.h"
#include <iostream>
#include <vector>
#include <array>
#include <chrono>

//Uniform buffer struct for mvp matrecies with explicit alignment specefied to match shader uniform alignment
// Could also use #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES but does not always catch alignment for nested structs
struct ModelViewProjectionUniformObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class UniformBufferData {
public:
	bool createDescriptorSetLayout(VkDevice logicalDevice);
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorPool(VkDevice logicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorSets(VkDevice logicalDevice, VkImageView imageView, VkSampler sampler, int maxFramesBeingProcessed);

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