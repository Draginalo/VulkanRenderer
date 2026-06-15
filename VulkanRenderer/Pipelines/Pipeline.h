#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <fstream>
#include <vector>

class Pipeline {
public:
	std::vector<char> readShaderFile(const char* filepath);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	virtual inline void cleanupPipeline(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
	}

	inline VkPipeline getPipeline() { return mPipeline; }
	inline VkPipelineLayout getPipelineLayout() { return mPipelineLayout; }

	virtual inline void bindPipeline(VkCommandBuffer commandBuffer) = 0;
protected:
	VkPipelineLayout mPipelineLayout;
	VkPipeline mPipeline;
};