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

	VkImageView* targetDepthImageView;
	VkImage* targetDepthImage;
	VkImageView* targetMSAA_ImageView;
	VkImage* targetMSAA_Image;
};

class GraphicsPipeline : public Pipeline {
public:
	bool createPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat, 
		VkFormat depthAttachmentFormat, ConfigurablePipelineValues configValues,
		const char* vertShaderFilepath, const char* fragShaderFilepath, VertexInputData vertexInputData, 
		std::vector<VkImageView> swapChainImageViews, bool usingDynamicRendering);
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
	bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
		VkImageView& swapChainImageView, uint32_t currFrame, void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*), void (*fpCmdEndRenderingKHR)(VkCommandBuffer)) override;

	inline void updateRenderExtents(VkExtent2D newExtents) { mRenderExtent = newExtents; }
private:
	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> mFramebuffers;

	VkImageView* mDepthImageView;
	VkImage* mDepthImage;

	VkImageView* mMSAA_ImageView;
	VkImage* mMSAA_Image;

	VkExtent2D mRenderExtent;

	VkSampleCountFlagBits mMSAA_PipelineSamples = VK_SAMPLE_COUNT_1_BIT;

	bool mDynamicRenderingEnabled = true;
};