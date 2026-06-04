#include "GraphicsPipeline.h"
#include "VertexBufferData.h"
#include "UniformBufferData.h"

std::vector<char> GraphicsPipeline::readShaderFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	std::vector<char> buffer = {};

	if (!file.is_open())
	{
		std::cout << "\nFailed to open file: " << filename << std::endl;
		return {};
	}

	size_t fileSize = (size_t)file.tellg();
	buffer.resize(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule GraphicsPipeline::createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = shaderByteCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderByteCode.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create shader module..." << std::endl;
		return nullptr;
	}

	return shaderModule;
}

bool GraphicsPipeline::createGraphicsPipeline(VkDevice logicalDevice, VkExtent2D viewportExtent, VkFormat colorAttachmentFormat,
	VkDescriptorSetLayout pDescriptorSetLayout)
{
	std::vector<char> basicTriVertShaderCode = readShaderFile("../../../../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv");
	std::vector<char> basicTriFragShaderCode = readShaderFile("../../../../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv");

	VkShaderModule basicTriVertShaderModule = createShaderModule(logicalDevice, basicTriVertShaderCode);
	VkShaderModule basicTriFragShaderModule = createShaderModule(logicalDevice, basicTriFragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = basicTriVertShaderModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = basicTriFragShaderModule;
	fragShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescription = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo assemblyInputInfo{};
	assemblyInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInputInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)viewportExtent.width;
	viewport.height = (float)viewportExtent.height;
	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;

	VkRect2D scissorRect{};
	scissorRect.offset = { 0, 0 };
	scissorRect.extent = viewportExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo{};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.lineWidth = 1.0;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Counter clockwise due to y coord being inverted
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.depthBiasConstantFactor = 0.0;
	rasterizationInfo.depthBiasClamp = 0.0;
	rasterizationInfo.depthBiasSlopeFactor = 0.0;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

	//VkPipelineDepthStencilStateCreateInfo here

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
	blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateCreateInfo.logicOpEnable = VK_TRUE;
	blendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	blendStateCreateInfo.attachmentCount = 1;
	blendStateCreateInfo.pAttachments = &colorBlendAttachment;
	blendStateCreateInfo.blendConstants[0] = 0.0f;
	blendStateCreateInfo.blendConstants[1] = 0.0f;
	blendStateCreateInfo.blendConstants[2] = 0.0f;
	blendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pSetLayouts = &pDescriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create pipeline layout..." << std::endl;
		return false;
	}

	VkPipelineRenderingCreateInfoKHR dynamicPipelineInfo{};
	dynamicPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	dynamicPipelineInfo.colorAttachmentCount = 1;
	dynamicPipelineInfo.pColorAttachmentFormats = &colorAttachmentFormat;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &assemblyInputInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState = &blendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = mPipelineLayout;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (!mDynamicRenderingEnabled) 
	{
		graphicsPipelineCreateInfo.renderPass = mRenderPass;
	}
	else
	{
		graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
		graphicsPipelineCreateInfo.pNext = &dynamicPipelineInfo;
	}

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, 
		&mGraphicsPipeline) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create graphics pipeline..." << std::endl;
		return false;
	}

	vkDestroyShaderModule(logicalDevice, basicTriVertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, basicTriFragShaderModule, nullptr);

	return true;
}

bool GraphicsPipeline::createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorAttachmentFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription{};
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachmentRef;

	//Dependencies to only allow image transitions to happen after the image has been retreived
	VkSubpassDependency subPassDepedency{};
	subPassDepedency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDepedency.dstSubpass = 0;
	subPassDepedency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDepedency.srcAccessMask = 0;
	subPassDepedency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDepedency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subPassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subPassDepedency;

	if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create render pass..." << std::endl;
		return false;
	}

	return true;
}

void GraphicsPipeline::cleanupPipeline(VkDevice logicalDevice)
{
	vkDestroyPipeline(logicalDevice, mGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);

	if (!mDynamicRenderingEnabled)
	{
		vkDestroyRenderPass(logicalDevice, mRenderPass, nullptr);
	}
}

bool GraphicsPipeline::createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkExtent2D extent)
{
	int imageViewCount = imageViews.size();

	mFramebuffers.resize(imageViewCount);

	for (int i = 0; i < imageViewCount; i++)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &imageViews[i];
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &mFramebuffers[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create framebuffer..." << std::endl;
			return false;
		}
	}

	return true;
}

void GraphicsPipeline::cleanupFrambuffers(VkDevice logicalDevice)
{
	for (VkFramebuffer framebuffer : mFramebuffers)
	{
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}
}

VkRenderPassBeginInfo GraphicsPipeline::getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent)
{
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = mRenderPass;
	renderPassBeginInfo.framebuffer = mFramebuffers[framebufferIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = renderPassExtent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	return renderPassBeginInfo;
}
