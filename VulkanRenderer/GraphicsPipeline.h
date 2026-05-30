#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <fstream>
#include <vector>

class GraphicsPipeline {
public:
	std::vector<char> readShaderFile(const std::string& filename);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode);

	bool createGraphicsPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent);
	bool createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat);

	void cleanupPipeline(VkDevice logicalDevice);
private:
	VkPipelineLayout mPipelineLayout;
	VkRenderPass mRenderPass;
	VkPipeline mGraphicsPipeline;
};