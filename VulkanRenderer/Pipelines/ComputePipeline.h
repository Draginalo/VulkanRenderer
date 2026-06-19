#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"
#include "../Mesh/Drawable.h"

#include <iostream>
#include <fstream>
#include <vector>

class ComputePipeline : public Pipeline {
public:
	ComputePipeline() : Pipeline(true) {}

	bool creatPipeline(VkDevice logicalDevice);
	void bindPipeline(VkCommandBuffer commandBuffer) override;
	bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
		VkImageView& swapChainImageView, uint32_t currFrame, void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*), void (*fpCmdEndRenderingKHR)(VkCommandBuffer)) override;
private:
};