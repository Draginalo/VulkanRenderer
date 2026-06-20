#include "ComputePipeline.h"

bool ComputePipeline::creatPipeline(VkDevice logicalDevice)
{
	if (*mPipelineDescriptorSetData.getDescriptorSetLayout() == VK_NULL_HANDLE) 
	{
		throw std::runtime_error("The pipeline descriptor set data must be added and initialized prior to creating the pipeline...");
	}

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = mPipelineDescriptorSetData.getDescriptorSetLayout();

	if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create compute pipeline layout..." << std::endl;
		return false;
	}

	auto computeShaderCode = readShaderFile("../Assets/Shaders/ByteEncoded/ComputeParticles_CS.spv");

	VkShaderModule computeShaderModule = createShaderModule(logicalDevice, computeShaderCode);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = mPipelineLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create compute pipeline..." << std::endl;
		return false;
	}

	vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);

	return true;
}

void ComputePipeline::bindPipeline(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
}

bool ComputePipeline::recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
	VkImageView& swapChainImageView, uint32_t currFrame, void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*), void (*fpCmdEndRenderingKHR)(VkCommandBuffer))
{
	vkCmdDispatch(commandBuffer, 256000 / 256, 1, 1);

	/*if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end compute command buffer recording..." << std::endl;
		return false;
	}*/

	return true;
}
