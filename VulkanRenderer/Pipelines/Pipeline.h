#pragma once

#include "vulkan/vulkan.h"
#include "VulkanRenderer/UniformDescriptorHandlers/DescriptorSetData.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <complex>
#include <initializer_list>
#include <iostream>
#include <fstream>
#include <vector>
#include "VulkanRenderer/Mesh/Drawable.h"

class Pipeline;

//Material struct for creating material based on a pipeline (for passing unique images, colors, etc)
struct Material {
	DescriptorSetData materialDescriptorSetData;
	Pipeline* pipelineForMaterial = nullptr;

	//Explicit padding for compiler warning, since this struct's size is not divisable by 8 in x86
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[4] = {};
	#endif
};

struct PipelineDependencyInfo {
	//TODO: Expand this to depend on multiple pipelines with other types of memory barriers that are not tied to a specific pipeline
	Pipeline* dependsOnPipeline = nullptr;
	std::vector<VkBufferMemoryBarrier2> buffMemBarriers;
};

class Pipeline {
public:
	Pipeline(bool isComputePipeline = false);
	virtual ~Pipeline() = default;

	std::vector<char> readShaderFile(const char* filepath);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	//Descriptor set data helper functions (to avoid having to call these functions from a modifiable reference 
	// to the descriptor set data)
	void loadPipelineDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors);

	void loadBaseMaterialDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors);

	void createPipelineDescriptorSetLayout(VkDevice logicalDevice);

	void createBaseMaterialDescriptorSetLayout(VkDevice logicalDevice);

	bool createBaseMaterialDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight);

	bool createPipelineDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight);

	void createPipelineMaterial(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors, VkDevice logicalDevice, VkDescriptorPool descriptorPool,
		BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight);

	virtual bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage,
		VkImageView& swapChainImageView, uint32_t currFrame, void(__stdcall* fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*),
		void(__stdcall* fpCmdEndRenderingKHR)(VkCommandBuffer)) = 0;

	virtual void cleanupPipeline(VkDevice logicalDevice);

	VkPipeline getPipeline() const;
	VkPipelineLayout getPipelineLayout() const;
	const std::vector<Material>* getPipelineMaterials() const;
	const Material* getBaseMaterial() const;
	//Returns as const so that the descriptor set data cannot be modified (only modified by loadPipelineDescriptorSetData())
	const DescriptorSetData* getPipelineDescriptorSetData() const;
	const UniformBufferDescriptor* getPipelineBufferDescriptor(uint32_t index) const;
	//Dev function (TODO: Remove this)
	UniformBufferDescriptor* getPipelineBufferDescriptorRef(uint32_t index);
	uint32_t getPipelineID() const;
	bool getIsComputePipeline() const;
	void setDependencyInfo(PipelineDependencyInfo dependencyInfo);
	PipelineDependencyInfo* getPipelineDependencyInfo();

	virtual void bindPipeline(VkCommandBuffer commandBuffer) = 0;

	Pipeline(const Pipeline&) = default;
protected:
	Material mBaseMaterial;
	DescriptorSetData mPipelineDescriptorSetData;

	VkPipelineLayout mPipelineLayout = {};
	VkPipeline mPipeline = {};

	PipelineDependencyInfo mDependencyInfo;

	std::vector<Material> mPipelineMaterials;

	uint32_t mPipelineID;
	static uint32_t mNumPipelineInstances;

	bool mIsComputePipeline = false;


	//Explicit padding for compiler warning, since there is 3 bytes of padding EXCEPT when this class' size is not 
	// divisable by 8 in x86
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[7] = {};
	#else
		uint8_t padding[3] = {};
	#endif
};