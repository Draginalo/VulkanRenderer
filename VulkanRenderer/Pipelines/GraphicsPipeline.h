#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"
#include "../Mesh/MeshGeneric.h"

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

class GraphicsPipeline : public Pipeline {
public:
	bool createPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat, 
		VkFormat depthAttachmentFormat, ConfigurablePipelineValues configValues,
		const char* vertShaderFilepath, const char* fragShaderFilepath, VertexInputData vertexInputData);
	bool createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat,
		VkSampleCountFlagBits samples);

	void cleanupPipeline(VkDevice logicalDevice) override;
	bool createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkImageView depthImageView, 
		VkImageView msaaImageView, VkExtent2D extent);
	void cleanupRenderPass(VkDevice logicalDevice);
	void cleanupFrambuffers(VkDevice logicalDevice);

	VkRenderPassBeginInfo getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent, 
		std::array<VkClearValue, 2> clearValues);

	inline const VkRenderPass getRenderPass() { return mRenderPass; }
	inline bool noLoadedFramebuffers() { return mFramebuffers.empty(); }

	inline bool dynamicRenderingEnabled() { return mDynamicRenderingEnabled; } 
	inline void setDynamicRenderingEnabled(bool enabled) { mDynamicRenderingEnabled = enabled; }

	void bindPipeline(VkCommandBuffer commandBuffer) override;

private:
	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> mFramebuffers;

	bool mDynamicRenderingEnabled = true;
};