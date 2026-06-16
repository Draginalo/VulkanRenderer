#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <array>
#include <chrono>

#include "Helpers/VertexInputData.h"
#include "UniformObjectHandlers/DescriptorSetData.h"

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

class UniformDescriptorManager {
public:
	void createPipelineSpecificDescriptorSets(std::vector<DescriptorSetData*> descriptorSets, VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, VkDescriptorPool descriptorPool, int maxFramesBeingProcessed, 
		const int numParticles, float recipAspect);

	void updatePipelineSpecificUniformBuffers(int currFrame, float aspectRatio, float dt);
	void bindGraphicsDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame);
	void bindComputeDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int currFrame);

	void bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles);
	bool addDataToSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, void* pData, 
		UniformBufferDescriptor* pDescriptorData);

	void cleanup(VkDevice logicalDevice);

	inline DescriptorSetData* getPipelineSpecificDescriptorSet(VkPipeline pipeline, int index = 0) 
	{ return mPipelineSpecificDescriptorSets[index]; };
private:
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize,
		int maxFramesBeingProcessed);

	bool createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, int maxFramesBeingProcessed,
		const int numParticles, float recipAspect);

	std::vector<VkBuffer> mUniformBuffers;
	std::vector<VkDeviceMemory> mUniformBuffersMemory;
	std::vector<void*> mUniformBuffersMapped;

	std::vector<VkDescriptorSet> mGlobalDescriptorSets;
	std::vector<DescriptorSetData*> mPipelineSpecificDescriptorSets;
	//std::unordered_map<VkPipeline, std::vector<DescriptorSetData*>> mPipelineSpecificDescriptorSets;
	//std::unordered_map<Material, std::vector<VkDescriptorSet>> mMaterialSpeecificDescriptorSets;
	//std::unordered_map<GameObject, std::vector<VkDescriptorSet>> mMaterialSpeecificDescriptorSets;

	std::vector<VkBuffer> mSSBOs;
	std::vector<VkDeviceMemory> mSSBOsMemory;
};