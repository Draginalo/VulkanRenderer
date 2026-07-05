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

#include "VulkanRenderer/Helpers/VertexInputData.h"
#include "DescriptorSetData.h"
#include "VulkanRenderer/Pipelines/Pipeline.h"

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
	bool createDescriptorPool(VkDevice logicalDevice, std::vector<Pipeline*> pipelines, uint32_t maxFramesInFlight);

	void createPipelineSpecificDescriptorSets(std::vector<Pipeline*> pipelines, VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, uint32_t maxFramesInFlight);

	void createMaterialSpecificDescriptorSets(std::vector<Material*> materials, VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, uint32_t maxFramesInFlight);

	void updatePipelineSpecificUniformBuffer(Pipeline* pipeline, uint32_t currFrame);
	void bindPipelineSpecificDescriptorSet(VkCommandBuffer commandBuffer, Pipeline* pipeline, uint32_t currFrame);
	void bindMaterialSpecificDescriptorSet(VkCommandBuffer commandBuffer, const Material* material, uint32_t currFrame);

	void bindSSBOs(VkCommandBuffer commandBuffer, uint32_t currFrame, uint32_t numParticles);
	bool addDataToSSBOs(VkDevice logicalDevice, void* pData, const UniformBufferDescriptor* pDescriptorData);

	void cleanup(VkDevice logicalDevice);

	std::vector<VkBuffer>* getPipelineDescriptorSSBO();

	//const DescriptorSetData* getPipelineSpecificDescriptorSet(Pipeline* pipeline)
	//{ return mPipelineSpecificDescriptorSets[pipeline->getPipelineID()]; };
private:
	bool createUniformBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize,
		uint32_t maxFramesInFlight);

	bool createSSBOs(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, uint32_t maxFramesInFlight);

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

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