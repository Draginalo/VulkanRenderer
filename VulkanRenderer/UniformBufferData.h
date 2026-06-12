#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan.h"
#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "Helpers/VertexInputHelpers.h"

//Uniform buffer struct for mvp matrecies with explicit alignment specefied to match shader uniform alignment
// Could also use #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES but does not always catch alignment for nested structs
struct ModelViewProjectionUniformObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct DeltaTimeUniformObject {
	float dt;
};

class UniformBufferData {
public:
	bool createDescriptorSetLayout(VkDevice logicalDevice);
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorPool(VkDevice logicalDevice, int maxFramesBeingProcessed);
	bool createDescriptorSets(VkDevice logicalDevice, VkImageView imageView, VkSampler sampler, int maxFramesBeingProcessed);

	bool createComputeDescriptorSetLayout(VkDevice logicalDevice);
	bool createComputeDescriptorSets(VkDevice logicalDevice, int maxFramesBeingProcessed, int numParticles);
	bool createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, int maxFramesBeingProcessed, 
		const int numParticles, float recipAspect, VkCommandPool commandPool, VkQueue submitQueue);

	void updateUniformBuffer(int currFrame, float aspectRatio, float dt);
	void bindGraphicsDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame);
	void bindComputeDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame);

	void bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles);

	void cleanup(VkDevice logicalDevice);
	inline VkDescriptorSetLayout getDescriptorSetLayout() { return mDescriptorSetLayout; }
	inline VkDescriptorSetLayout getComputeDescriptorSetLayout() { return mComputeDescriptorSetLayout; }
private:
	VkDescriptorSetLayout mDescriptorSetLayout;
	VkDescriptorSetLayout mComputeDescriptorSetLayout;

	std::vector<VkBuffer> mUniformBuffersMVP;
	std::vector<VkDeviceMemory> mUniformBuffersMVPMemory;
	std::vector<void*> mUniformBuffersMVPMapped;

	std::vector<VkBuffer> mUniformBuffersDT;
	std::vector<VkDeviceMemory> mUniformBuffersDTMemory;
	std::vector<void*> mUniformBuffersDTMapped;

	VkDescriptorPool mDescriptorPool;
	std::vector<VkDescriptorSet> mDescriptorSets;
	std::vector<VkDescriptorSet> mComputeDescriptorSets;

	std::vector<VkBuffer> mSSBOs;
	std::vector<VkDeviceMemory> mSSBOsMemory;
};