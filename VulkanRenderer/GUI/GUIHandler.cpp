#include "GUIHandler.h"

void GUIHandler::initImGui(GLFWwindow* window, VulkanManager* vulkanManager)
{
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets = 1000;
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCreateInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(vulkanManager->getLogicalDevice(), &poolCreateInfo, nullptr, &mImGuiDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to make ImGui descriptor pool...");
	}

	ImGui::CreateContext();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.DisplaySize.x = (float)width;
	io.DisplaySize.y = (float)height;

	if (io.BackendPlatformUserData == nullptr)
	{
		ImGui_ImplGlfw_InitForVulkan(window, true);
	}

	ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
	pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineInfo.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	pipelineInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = vulkanManager->getSwapChainImageFormatRef();
	pipelineInfo.PipelineRenderingCreateInfo.depthAttachmentFormat = vulkanManager->getDepthFormat();
	pipelineInfo.MSAASamples = vulkanManager->getMSAA_Samples();

	if (!vulkanManager->getGraphicsPipeline()->dynamicRenderingEnabled())
	{
		pipelineInfo.RenderPass = vulkanManager->getGraphicsPipeline()->getRenderPass();
	}

	SwapChainSupportDetails swapChainSupportDetails = vulkanManager->querySwapChainSupport(vulkanManager->getPhysicalDevice());

	uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainSupportDetails.capabilities.maxImageCount;
	}

	imageCount = swapChainSupportDetails.capabilities.minImageCount;

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = vulkanManager->getInstance();
	initInfo.PhysicalDevice = vulkanManager->getPhysicalDevice();
	initInfo.Device = vulkanManager->getLogicalDevice();
	initInfo.Queue = vulkanManager->getGraphicsQueue();
	initInfo.DescriptorPool = mImGuiDescriptorPool;
	initInfo.MinImageCount = swapChainSupportDetails.capabilities.minImageCount;
	initInfo.ImageCount = imageCount;
	initInfo.UseDynamicRendering = vulkanManager->getGraphicsPipeline()->dynamicRenderingEnabled();
	initInfo.PipelineInfoMain = pipelineInfo;

	ImGui_ImplVulkan_Init(&initInfo);
}

void GUIHandler::checkGUI_State(GLFWwindow* window, VulkanManager* vulkanManager)
{
	if (mNeedToReloadGUI)
	{
		cleanupGUI(vulkanManager->getLogicalDevice());
		initImGui(window, vulkanManager);
		mNeedToReloadGUI = false;
	}
}

void GUIHandler::cleanupGUI(VkDevice logicalDevice)
{
	vkDeviceWaitIdle(logicalDevice);

	ImGui_ImplVulkan_Shutdown();

	vkDestroyDescriptorPool(logicalDevice, mImGuiDescriptorPool, nullptr);
}