#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"
#include "VulkanRenderer/Mesh/MeshGeneric.h"

#include <iostream>
#include <fstream>
#include <vector>

//Struct to pass the main configurable values when creating the pipeline (rather than moving all the create info 
// structs as parameters). Might need to refactor to have a better way of creating different pipelines
struct ConfigurablePipelineValues {
	VkImageView* targetDepthImageView;
	VkImage* targetDepthImage;
	VkImageView* targetMSAA_ImageView;
	VkImage* targetMSAA_Image;

	VkPrimitiveTopology primitiveTopology;
	VkBool32 depthWriteEnabled;
	VkSampleCountFlagBits samples;

	//Explicit padding for x64 since struct size is not divisable by 8 in x64
	#if defined(_WIN64) || defined(__x86_64__)
		uint8_t padding[4];
	#endif
};

class GraphicsPipeline : public Pipeline {
public:
	~GraphicsPipeline() {}

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

	VkRenderPass getRenderPass() const;
	bool noLoadedFramebuffers() const;

	bool dynamicRenderingEnabled() const;
	void setDynamicRenderingEnabled(bool enabled);

	void bindPipeline(VkCommandBuffer commandBuffer) override;
	bool recordPipelineCommands(VkCommandBuffer commandBuffer, const Drawable* drawable, VkImage& swapChainImage, 
		VkImageView& swapChainImageView, uint32_t currFrame, 
		void (__stdcall *fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*), 
		void (__stdcall *fpCmdEndRenderingKHR)(VkCommandBuffer)) override;

	void updateRenderExtents(VkExtent2D newExtents);
private:
	VkRenderPass mRenderPass = VK_NULL_HANDLE;

	VkImageView* mDepthImageView = nullptr;
	VkImage* mDepthImage = nullptr;

	VkImageView* mMSAA_ImageView = nullptr;
	VkImage* mMSAA_Image = nullptr;

	std::vector<VkFramebuffer> mFramebuffers;

	VkExtent2D mRenderExtent = {};

	VkSampleCountFlagBits mMSAA_PipelineSamples = VK_SAMPLE_COUNT_1_BIT;

	bool mDynamicRenderingEnabled = true;

	//Explicit padding for compiler warning (3 bits of padding EXCEPT for x86 release mode (when size of a vector 
	// is only devisible by 4 and not divisable by 8))
	#if ((defined(_M_IX86) || defined(__i386__)) && !defined(_DEBUG))
		uint8_t padding[7] = {};
	#else
		uint8_t padding[3] = {};
	#endif
};