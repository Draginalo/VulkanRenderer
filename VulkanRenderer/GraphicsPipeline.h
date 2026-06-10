#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <fstream>
#include <vector>

class GraphicsPipeline {
public:
	std::vector<char> readShaderFile(const std::string& filepath);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	bool createGraphicsPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat, 
		VkFormat depthAttachmentFormat, VkDescriptorSetLayout descriptorSetLayout, VkSampleCountFlagBits samples, 
		bool renderingParticles);
	bool createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat,
		VkSampleCountFlagBits samples);

	void cleanupPipeline(VkDevice logicalDevice);
	bool createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkImageView depthImageView, 
		VkImageView msaaImageView, VkExtent2D extent);
	void cleanupFrambuffers(VkDevice logicalDevice);

	VkRenderPassBeginInfo getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent, 
		std::array<VkClearValue, 2> clearValues);

	inline VkPipeline getGraphicsPipeline() { return mGraphicsPipeline; }
	inline VkPipeline getComputePipeline() { return mComputePipeline; }
	inline VkPipelineLayout getGraphicsPipelineLayout() { return mGraphicsPipelineLayout; }
	inline VkPipelineLayout getComputePipelineLayout() { return mComputePipelineLayout; }

	inline bool dynamicRenderingEnabled() { return mDynamicRenderingEnabled; }

	bool createComputePipeline(VkDevice logicalDevice, VkDescriptorSetLayout descriptorSetLayout);

private:
	VkPipelineLayout mGraphicsPipelineLayout;
	VkRenderPass mRenderPass;
	VkPipeline mGraphicsPipeline;
	std::vector<VkFramebuffer> mFramebuffers;

	VkPipelineLayout mComputePipelineLayout;
	VkPipeline mComputePipeline;

	bool mDynamicRenderingEnabled = true;
};