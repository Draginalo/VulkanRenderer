#include "GraphicsPipeline.h"
#include "VertexBufferData.h"
#include "UniformBufferData.h"

std::vector<char> GraphicsPipeline::readShaderFile(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	std::vector<char> buffer = {};

	if (!file.is_open())
	{
		std::cout << "\nFailed to open file: " << filepath << std::endl;
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
	VkFormat depthAttachmentFormat, VkDescriptorSetLayout descriptorSetLayout, VkSampleCountFlagBits samples, bool renderingParticles)
{
	std::vector<char> vertShaderCode;
	std::vector<char> fragShaderCode;

	if (renderingParticles)
	{
		vertShaderCode = readShaderFile("../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv");
		fragShaderCode = readShaderFile("../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv");
	}
	else 
	{
		vertShaderCode = readShaderFile("../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv");
		fragShaderCode = readShaderFile("../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv");
	}

	VkShaderModule vertShaderModule = createShaderModule(logicalDevice, vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(logicalDevice, fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = vertShaderModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = fragShaderModule;
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

	VkVertexInputBindingDescription vertexBindingDescription;
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription;

	VkPipelineInputAssemblyStateCreateInfo assemblyInputInfo{};
	assemblyInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInputInfo.primitiveRestartEnable = VK_FALSE;

	if (renderingParticles)
	{
		assemblyInputInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		vertexBindingDescription = Particle::getBindingDescription();
		vertexAttributeDescription = Particle::getAttributeDescriptions();
	}
	else 
	{
		vertexBindingDescription = Vertex::getBindingDescription();
		vertexAttributeDescription = Vertex::getAttributeDescriptions();
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

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
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.rasterizationSamples = samples;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	//Fixes transparency when rendering particles
	if (renderingParticles)
	{
		depthStencilInfo.depthWriteEnable = VK_FALSE;
	}

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
	blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateCreateInfo.logicOpEnable = VK_FALSE;
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
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &mGraphicsPipelineLayout) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create graphics pipeline layout..." << std::endl;
		return false;
	}

	VkPipelineRenderingCreateInfoKHR dynamicPipelineInfo{};
	dynamicPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	dynamicPipelineInfo.colorAttachmentCount = 1;
	dynamicPipelineInfo.pColorAttachmentFormats = &colorAttachmentFormat;
	dynamicPipelineInfo.depthAttachmentFormat = depthAttachmentFormat;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &assemblyInputInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &blendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = mGraphicsPipelineLayout;
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

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);

	return true;
}

bool GraphicsPipeline::createRenderPass(VkDevice logicalDevice, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, 
	VkSampleCountFlagBits samples)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorAttachmentFormat;
	colorAttachment.samples = samples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthAttachmentFormat;
	depthAttachment.samples = samples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription msaaResolveAttachment{};
	msaaResolveAttachment.format = colorAttachmentFormat;
	msaaResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	msaaResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	msaaResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	msaaResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	msaaResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	msaaResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	msaaResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference msaaResolveAttachmentRef{};
	msaaResolveAttachmentRef.attachment = 2;
	msaaResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription{};
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachmentRef;
	subPassDescription.pDepthStencilAttachment = &depthAttachmentRef;
	subPassDescription.pResolveAttachments = &msaaResolveAttachmentRef;

	//Dependencies to only allow image transitions to happen after the image has been retreived
	VkSubpassDependency subPassDepedency{};
	subPassDepedency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDepedency.dstSubpass = 0;
	subPassDepedency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subPassDepedency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subPassDepedency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subPassDepedency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, msaaResolveAttachment };
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
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
	vkDestroyPipelineLayout(logicalDevice, mGraphicsPipelineLayout, nullptr);

	if (!mDynamicRenderingEnabled)
	{
		vkDestroyRenderPass(logicalDevice, mRenderPass, nullptr);
	}

	vkDestroyPipeline(logicalDevice, mComputePipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, mComputePipelineLayout, nullptr);
}

bool GraphicsPipeline::createFramebuffers(VkDevice logicalDevice, std::vector<VkImageView> imageViews, VkImageView depthImageView, 
	VkImageView msaaImageView, VkExtent2D extent)
{
	int imageViewCount = imageViews.size();

	mFramebuffers.resize(imageViewCount);

	for (int i = 0; i < imageViewCount; i++)
	{
		std::array<VkImageView, 3> attatchments = { msaaImageView, depthImageView, imageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attatchments.size());
		framebufferInfo.pAttachments = attatchments.data();
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

VkRenderPassBeginInfo GraphicsPipeline::getRenderPassBeginInfo(uint32_t framebufferIndex, VkExtent2D renderPassExtent,
	std::array<VkClearValue, 2> clearValues)
{
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = mRenderPass;
	renderPassBeginInfo.framebuffer = mFramebuffers[framebufferIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = renderPassExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	return renderPassBeginInfo;
}

bool GraphicsPipeline::createComputePipeline(VkDevice logicalDevice, VkDescriptorSetLayout descriptorSetLayout)
{
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &mComputePipelineLayout) != VK_SUCCESS)
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
	pipelineInfo.layout = mComputePipelineLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mComputePipeline) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create compute pipeline..." << std::endl;
		return false;
	}

	vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);

	return true;
}
