#pragma once

#include "vulkan/vulkan.h"
#include "Mesh/MeshGeneric.h"

#include <iostream>
#include <fstream>
#include <vector>

//Struct to pass the main configurable values when creating the pipeline (rather than moving all the create info 
// structs as parameters). Might need to refactor to have a better way of creating different pipelines
struct ConfigurablePipelineValues {
	VkPrimitiveTopology primitiveTopology;
	VkBool32 depthWriteEnabled;
	VkSampleCountFlagBits samples;
};

class PipelineData {
public:
	std::vector<char> readShaderFile(const char* filepath);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	bool createGraphicsPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat, 
		VkFormat depthAttachmentFormat, VkDescriptorSetLayout descriptorSetLayout, ConfigurablePipelineValues configValues,
		const char* vertShaderFilepath, const char* fragShaderFilepath, VertexInputData vertexInputData);
	bool createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat,
		VkSampleCountFlagBits samples);

	void cleanupPipeline(VkDevice logicalDevice);
	bool createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkImageView depthImageView, 
		VkImageView msaaImageView, VkExtent2D extent);
	void cleanupRenderPass(VkDevice logicalDevice);
	void cleanupFrambuffers(VkDevice logicalDevice);

	VkRenderPassBeginInfo getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent, 
		std::array<VkClearValue, 2> clearValues);

	inline VkPipeline getGraphicsPipeline() { return mGraphicsPipeline; }
	inline VkPipeline getComputePipeline() { return mComputePipeline; }
	inline VkPipelineLayout getGraphicsPipelineLayout() { return mGraphicsPipelineLayout; }
	inline VkPipelineLayout getComputePipelineLayout() { return mComputePipelineLayout; }
	inline const VkRenderPass getRenderPass() { return mRenderPass; }
	inline bool noLoadedFramebuffers() { return mFramebuffers.empty(); }

	inline bool dynamicRenderingEnabled() { return mDynamicRenderingEnabled; }
	inline void setDynamicRenderingEnabled(bool enabled) { mDynamicRenderingEnabled = enabled; }

	bool createComputePipeline(VkDevice logicalDevice, VkDescriptorSetLayout descriptorSetLayout);

private:
	VkPipelineLayout mGraphicsPipelineLayout;
	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	VkPipeline mGraphicsPipeline;
	std::vector<VkFramebuffer> mFramebuffers;

	VkPipelineLayout mComputePipelineLayout;
	VkPipeline mComputePipeline;

	bool mDynamicRenderingEnabled = true;
};