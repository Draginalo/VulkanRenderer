#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <fstream>
#include <vector>

class GraphicsPipeline {
public:
	std::vector<char> readShaderFile(const std::string& filename);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	bool createGraphicsPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat, 
		VkDescriptorSetLayout pDescriptorSetLayout);
	bool createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat);

	void cleanupPipeline(VkDevice logicalDevice);
	bool createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkExtent2D extent);
	void cleanupFrambuffers(VkDevice logicalDevice);

	VkRenderPassBeginInfo getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent);
	inline VkPipeline getPipeline() { return mGraphicsPipeline; }
	inline VkPipelineLayout getPipelineLayout() { return mPipelineLayout; }

	inline bool dynamicRenderingEnabled() { return mDynamicRenderingEnabled; }

private:
	VkPipelineLayout mPipelineLayout;
	VkRenderPass mRenderPass;
	VkPipeline mGraphicsPipeline;
	std::vector<VkFramebuffer> mFramebuffers;

	bool mDynamicRenderingEnabled = true;
};