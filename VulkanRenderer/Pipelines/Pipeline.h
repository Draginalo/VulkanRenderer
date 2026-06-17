#pragma once

#include "vulkan/vulkan.h"
#include "../UniformObjectHandlers/DescriptorSetData.h"

#include <iostream>
#include <fstream>
#include <vector>

//Material struct for creating material based on a pipeline (for passing unique images, colors, etc)
struct Material {
	DescriptorSetData materialDescriptorSetData;
};

class Pipeline {
public:
	Pipeline(bool isComputePipeline = false) : mIsComputePipeline(isComputePipeline), mPipelineID(mNumPipelineInstances++) {}

	std::vector<char> readShaderFile(const char* filepath);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	//Descriptor set data helper functions (to avoid having to call these functions from a modifiable reference 
	// to the descriptor set data)
	inline void loadPipelineDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors, int maxFramesInFlight) 
	{ 
		mPipelineDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors, maxFramesInFlight); 
	}

	inline void loadBaseMaterialDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors, int maxFramesInFlight)
	{
		mPipelineDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors, maxFramesInFlight);
	}

	inline void createPipelineDescriptorSetLayout(VkDevice logicalDevice) 
	{ 
		mPipelineDescriptorSetData.createDescriptorSetLayout(logicalDevice); 
	}

	inline bool createBaseMaterialDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, int maxFramesInFlight) 
	{
		return mBaseMaterialDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool, destUniformBuffers, 
			destStorageBuffers, maxFramesInFlight);
	}

	inline bool createPipelineDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, int maxFramesInFlight)
	{
		return mPipelineDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool, destUniformBuffers, destStorageBuffers,
			maxFramesInFlight);
	}

	void createPipelineMaterial(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors, VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, int maxFramesInFlight);

	virtual inline void cleanupPipeline(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
		mPipelineDescriptorSetData.cleanup(logicalDevice);
	}

	inline VkPipeline getPipeline() { return mPipeline; }
	inline VkPipelineLayout getPipelineLayout() { return mPipelineLayout; }

	//Returns as const so that the descriptor set data cannot be modified (only modified by loadPipelineDescriptorSetData())
	inline const DescriptorSetData* getPipelineDescriptorSetData() const { return &mPipelineDescriptorSetData; }

	inline const UniformBufferDescriptor* getPipelineBufferDescriptor(int index) const { 
		return &(*mPipelineDescriptorSetData.getUniformBufferDescriptors())[index]; }

	//Dev function (TODO: Remove this)
	inline UniformBufferDescriptor* getPipelineBufferDescriptorRef(int index) {
		return &(*mPipelineDescriptorSetData.getUniformBufferDescriptorsRef())[index];
	}

	inline uint32_t getPipelineID() { return mPipelineID; }
	inline bool getIsComputePipeline() { return mIsComputePipeline; }

	virtual inline void bindPipeline(VkCommandBuffer commandBuffer) = 0;
protected:
	VkPipelineLayout mPipelineLayout = {};
	VkPipeline mPipeline = {};

	DescriptorSetData mPipelineDescriptorSetData;
	DescriptorSetData mBaseMaterialDescriptorSetData;

	std::vector<Material> mPipelineMaterials;

	bool mIsComputePipeline = false;

	uint32_t mPipelineID;
	static uint32_t mNumPipelineInstances;
};