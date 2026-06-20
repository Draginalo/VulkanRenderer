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
#include "UniformDescriptorHandlers/DescriptorSetData.h"
#include "Pipelines/Pipeline.h"

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
	bool createDescriptorPool(VkDevice logicalDevice, std::vector<Pipeline*> pipelines, int maxFramesInFlight);

	void createPipelineSpecificDescriptorSets(std::vector<Pipeline*> pipelines, VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, int maxFramesInFlight);

	void createMaterialSpecificDescriptorSets(std::vector<Material*> materials, VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, int maxFramesInFlight);

	void updatePipelineSpecificUniformBuffer(Pipeline* pipeline, int currFrame);
	void bindPipelineSpecificDescriptorSet(VkCommandBuffer commandBuffer, Pipeline* pipeline, int currFrame);
	void bindMaterialSpecificDescriptorSet(VkCommandBuffer commandBuffer, const Material* material, int currFrame);

	void bindSSBOs(VkCommandBuffer commandBuffer, int currFrame, int numParticles);
	bool addDataToSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, void* pData, 
		const UniformBufferDescriptor* pDescriptorData);

	void cleanup(VkDevice logicalDevice);

	std::vector<VkBuffer>* getPipelineDescriptorSSBO() { return &mPipelineDescriptorSSBOs; }

	//inline const DescriptorSetData* getPipelineSpecificDescriptorSet(Pipeline* pipeline)
	//{ return mPipelineSpecificDescriptorSets[pipeline->getPipelineID()]; };
private:
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize,

		int maxFramesInFlight);

	bool createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, int maxFramesInFlight);

	VkDescriptorPool mDescriptorPool;

	std::vector<VkDescriptorSet> mGlobalDescriptorSets;

	//Pipeline Descriptor Set Buffers
	std::vector<VkBuffer> mPipelineUniformBuffers;
	std::vector<VkDeviceMemory> mPipelineUniformBuffersMemory;
	std::vector<void*> mPipelineUniformBuffersMapped;

	std::vector<VkBuffer> mPipelineDescriptorSSBOs;
	std::vector<VkDeviceMemory> mPipelineDescriptorSSBOsMemory;

	//Material Descriptor Set Buffers
	std::vector<VkBuffer> mMaterialUniformBuffers;
	std::vector<VkDeviceMemory> mMaterialUniformBuffersMemory;
	std::vector<void*> mMaterialUniformBuffersMapped;

	std::vector<VkBuffer> mMaterialDescriptorSSBOs;
	std::vector<VkDeviceMemory> mMaterialDescriptorSSBOsMemory;
};