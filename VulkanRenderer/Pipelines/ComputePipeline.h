#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"
#include "VulkanRenderer/Mesh/Drawable.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>

class ComputePipeline : public Pipeline {
public:
	ComputePipeline();

	bool creatPipeline(VkDevice logicalDevice, glm::uvec3 groupCount, glm::uvec3 groupCountDivisor);
	void bindPipeline(VkCommandBuffer commandBuffer) override;
	bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
		VkImageView& swapChainImageView, uint32_t currFrame, void (__stdcall *fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*),
		void (__stdcall *fpCmdEndRenderingKHR)(VkCommandBuffer)) override;
private:
	glm::uvec3 mGroupCount = { 0.0, 0.0, 0.0 };
	glm::uvec3 mGroupCountDivisor = { 0.0, 0.0, 0.0 };
};