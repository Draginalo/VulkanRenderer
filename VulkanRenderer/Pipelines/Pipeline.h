#pragma once

#include "vulkan/vulkan.h"
#include "../UniformDescriptorHandlers/DescriptorSetData.h"

#include <iostream>
#include <fstream>
#include <vector>
#include "../Mesh/Drawable.h"

class Pipeline;

//Material struct for creating material based on a pipeline (for passing unique images, colors, etc)
struct Material {
	DescriptorSetData materialDescriptorSetData;
	Pipeline* pipelineForMaterial;
};

struct PipelineDependencyInfo {
	//TODO: Expand this to depend on multiple pipelines with other types of memory barriers that are not tied to a specific pipeline
	Pipeline* dependsOnPipeline = nullptr;
	std::vector<VkBufferMemoryBarrier2> buffMemBarriers;
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
	{ mBaseMaterial.materialDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors, maxFramesInFlight); }

	inline void createPipelineDescriptorSetLayout(VkDevice logicalDevice) 
	{ mPipelineDescriptorSetData.createDescriptorSetLayout(logicalDevice); }

	inline void createBaseMaterialDescriptorSetLayout(VkDevice logicalDevice)
	{ 
		//Only creates the descriptor set data if there are descriptors to add
		if (mBaseMaterial.materialDescriptorSetData.getTotalDescriptorsForMaterial() != 0)
		{
			mBaseMaterial.materialDescriptorSetData.createDescriptorSetLayout(logicalDevice);
		}
	}

	inline bool createBaseMaterialDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, int maxFramesInFlight) 
	{
		mBaseMaterial.pipelineForMaterial = this;

		//Only creates the descriptor set data if there are descriptors to add
		if (mBaseMaterial.materialDescriptorSetData.getTotalDescriptorsForMaterial() == 0) { return true;} 

		return mBaseMaterial.materialDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool, destUniformBuffers,
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

	virtual bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
		VkImageView& swapChainImageView, uint32_t currFrame, void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*), void (*fpCmdEndRenderingKHR)(VkCommandBuffer)) = 0;

	virtual inline void cleanupPipeline(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
		mPipelineDescriptorSetData.cleanup(logicalDevice);

		mBaseMaterial.materialDescriptorSetData.cleanup(logicalDevice);

		for (const Material& material : mPipelineMaterials)
		{
			material.materialDescriptorSetData.cleanup(logicalDevice);
		}

		mPipelineMaterials.clear();
	}

	inline VkPipeline getPipeline() { return mPipeline; }
	inline VkPipelineLayout getPipelineLayout() { return mPipelineLayout; }
	inline const std::vector<Material>* getPipelineMaterials() const { return &mPipelineMaterials; }
	inline const Material* getBaseMaterial() const { return &mBaseMaterial; }

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

	inline void setDependencyInfo(PipelineDependencyInfo dependencyInfo) { mDependencyInfo = dependencyInfo; } 
	inline PipelineDependencyInfo* getPipelineDependencyInfo() { return &mDependencyInfo; }

	virtual inline void bindPipeline(VkCommandBuffer commandBuffer) = 0;
protected:
	VkPipelineLayout mPipelineLayout = {};
	VkPipeline mPipeline = {};

	DescriptorSetData mPipelineDescriptorSetData;
	Material mBaseMaterial;

	std::vector<Material> mPipelineMaterials;

	bool mIsComputePipeline = false;

	PipelineDependencyInfo mDependencyInfo;

	uint32_t mPipelineID;
	static uint32_t mNumPipelineInstances;
};