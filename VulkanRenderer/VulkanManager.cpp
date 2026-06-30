#include "VulkanManager.h"
#include "Helpers/BufferHelpers.h"
#include "UniformDescriptorHandlers//UniformDescriptors/UniformBufferDescriptor.h"
#include "UniformDescriptorHandlers/UniformDescriptors/UniformImageDescriptor.h"

#include <random>

bool VulkanManager::initVulkan(GLFWwindow* window)
{
	std::cout << "\nInitializing Vulkan" << std::endl;

	//Window set up
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	//Vulkan set up
	createInstance();
	setUpDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createSwapChainImageViews();
	createCommandPool();
	createCommandBuffers();

	//Creates global color and depth attachments
	createDepthResources();
	createMSAA_ColorResources();

	//Creates synchronization objects and adds extension function pointers
	createSyncObjects();
	registerExtensionFunctions(mInstance);

	//Loads texture for model (TODO: Combine this with model loading in a dedicated model loader to save the texture and model in a 
	// storage list somewhere lfor assigning pointers to those assets)
	uint32_t texMipLevels;
	createTextureImage(mLogicalDevice, mPhysicalDevice, mTextureImage, mTextureMemory, mCommandPool, mGraphicsQueue,
		"../Assets/Models/Room/room.png", texMipLevels);

	if (!createImageView(mLogicalDevice, mTextureImage, &mTextureImageView, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT, texMipLevels) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create texture image view..." << std::endl;
		return false;
	}

	//Creates texture sampler for mesh
	createTextureSampler(mLogicalDevice, mPhysicalDevice, &mTextureSampler);

	//Creates the house mesh
	mHouseMesh.createVertexDataFromModel(mLogicalDevice, mPhysicalDevice, mCommandPool, mGraphicsQueue, "../Assets/Models/Room/room.obj");

	////Manual defining of the uniform descriptors for the pipelines to be used

	std::vector<UniformBufferDescriptor> scene1BuffDescriptors;

	//Defines model view projection uniform for the model in scene 1.
	// TODO: Make the VP part into a global uniform and then add a push constant for the model matrix of each model/GameObject
	UniformBufferDescriptor mvpDescriptor{};
	mvpDescriptor.setDstBinding(0);
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(ModelViewProjectionUniformObject);
	mvpDescriptor.setBufferInfo(bufferInfo);
	mvpDescriptor.setDataPointer(&mMVP_UniformObject);
	scene1BuffDescriptors.push_back(mvpDescriptor);

	//Defines texture uniform descriptor for base material of the pipeline that will render the model
	std::vector<UniformImageDescriptor> scene1Material_ImgDescriptors;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = mTextureImageView;
	imageInfo.sampler = mTextureSampler;

	UniformImageDescriptor meshMaterialTextureDescriptor{};
	meshMaterialTextureDescriptor.setImageInfo(imageInfo);
	meshMaterialTextureDescriptor.setDstBinding(1);
	scene1Material_ImgDescriptors.push_back(meshMaterialTextureDescriptor);

	std::vector<UniformBufferDescriptor> scene2BuffComputeDescriptors;

	//Defines the uniform descriptor for delta time for the compute scene
	UniformBufferDescriptor dtDescriptor{};
	dtDescriptor = UniformBufferDescriptor(PIPELINE_SPECIFIC, 0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
		sizeof(DeltaTimeUniformObject), &mDtUniformObject);
	scene2BuffComputeDescriptors.push_back(dtDescriptor);

	//Defines the descriptor for the previous compute particle data
	UniformBufferDescriptor prevParticleDescriptor{};
	prevParticleDescriptor = UniformBufferDescriptor(PIPELINE_SPECIFIC, 1, VK_SHADER_STAGE_COMPUTE_BIT, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sizeof(Particle2D) * PARTICLE_COUNT, nullptr, true);
	scene2BuffComputeDescriptors.push_back(prevParticleDescriptor);

	//Defines the descriptor for the current particle data
	UniformBufferDescriptor currParticleDescriptor{};
	currParticleDescriptor = UniformBufferDescriptor(PIPELINE_SPECIFIC, 2, VK_SHADER_STAGE_COMPUTE_BIT, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sizeof(Particle2D) * PARTICLE_COUNT);
	scene2BuffComputeDescriptors.push_back(currParticleDescriptor);

	//Creates scene 1 graphics pipeline and saves it as a pointer to load and create it's data
	mGraphicsPipelineStorageList.push_back({});
	GraphicsPipeline* scene1GraphicsPipeline = &mGraphicsPipelineStorageList[mGraphicsPipelineStorageList.size() - 1];

	scene1GraphicsPipeline->loadPipelineDescriptorSetData(scene1BuffDescriptors, {}, MAX_FRAMES_IN_FLIGHT);
	scene1GraphicsPipeline->loadBaseMaterialDescriptorSetData({}, scene1Material_ImgDescriptors, MAX_FRAMES_IN_FLIGHT);

	//Creates scene 2 graphics pipeline and saves it as a pointer to load and create it's data
	mGraphicsPipelineStorageList.push_back({});
	GraphicsPipeline* scene1GraphicsPipeline2 = &mGraphicsPipelineStorageList[mGraphicsPipelineStorageList.size() - 1];

	scene1GraphicsPipeline2->loadPipelineDescriptorSetData({}, {}, MAX_FRAMES_IN_FLIGHT);
	scene1GraphicsPipeline2->loadBaseMaterialDescriptorSetData({}, {}, MAX_FRAMES_IN_FLIGHT);

	//Creates scene 1 compute pipeline and saves it as a pointer to load and create it's data
	mComputePipelineStorageList.push_back({});
	ComputePipeline* scene2ComputePipeline = &mComputePipelineStorageList[mComputePipelineStorageList.size() - 1];
	scene2ComputePipeline->loadPipelineDescriptorSetData(scene2BuffComputeDescriptors, {}, MAX_FRAMES_IN_FLIGHT);

	//Adds all the created scenes to a vector of all the scenes to be created
	std::vector<Pipeline*> allPipelines;
	for (int i = 0; i < mGraphicsPipelineStorageList.size(); i++)
	{
		allPipelines.push_back(&mGraphicsPipelineStorageList[i]);
	}

	for (int i = 0; i < mComputePipelineStorageList.size(); i++)
	{
		allPipelines.push_back(&mComputePipelineStorageList[i]);
	}

	//Creates the descriptor pool and descriptor sets for all the pipelines to be created
	mUniformDescriptorManager.createDescriptorPool(mLogicalDevice, allPipelines, MAX_FRAMES_IN_FLIGHT);
	mUniformDescriptorManager.createPipelineSpecificDescriptorSets(allPipelines, 
		mLogicalDevice, mPhysicalDevice, MAX_FRAMES_IN_FLIGHT);
	//mGraphicsPipeline.createPipelineMaterial()


	//Defiens config values for scene 1 graphics pipeline creation, including shaders and the vertex input data for the vertex shader
	ConfigurablePipelineValues configValues{};
	configValues.samples = mMSAA_Samples;
	configValues.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configValues.depthWriteEnabled = true;
	configValues.targetDepthImage = &mDepthImage;
	configValues.targetDepthImageView = &mDepthImageView;
	configValues.targetMSAA_Image = &mMSAA_ColorImage;
	configValues.targetMSAA_ImageView = &mMSAA_ColorImageView;

	char* vertShader = "../Assets/Shaders/ByteEncoded/RenderModel_VS.spv";
	char* fragShader = "../Assets/Shaders/ByteEncoded/RenderModel_FS.spv";

	VertexInputData vertexInputInfo = mHouseMesh.getVertexInputData();

	mGraphicsPipelineStorageList[0].createPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat, configValues,
		vertShader, fragShader, vertexInputInfo, mSwapChainImageViews, mUsingDynamicRendering);

	//Defiens config values for scene 2 graphics pipeline creation
	configValues = {};
	configValues.samples = mMSAA_Samples;
	configValues.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	configValues.depthWriteEnabled = false;
	configValues.targetDepthImage = &mDepthImage;
	configValues.targetDepthImageView = &mDepthImageView;
	configValues.targetMSAA_Image = &mMSAA_ColorImage;
	configValues.targetMSAA_ImageView = &mMSAA_ColorImageView;

	vertShader = "../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv";
	fragShader = "../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv";

	vertexInputInfo = Particle2D::getParticleInputData();

	mGraphicsPipelineStorageList[1].createPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat, configValues,
		vertShader, fragShader, vertexInputInfo, mSwapChainImageViews, mUsingDynamicRendering);

	//Creates scene 2 compute pipeline
	mComputePipelineStorageList[0].creatPipeline(mLogicalDevice, glm::uvec3(PARTICLE_COUNT, 1, 1), glm::uvec3(256, 1, 1));

	//Marks the scene 2 graphics pipeline to depend on the scene 2 compute pipeline because it renders the particles after the 
	// compute pipeline modifies them (TODO: Make this automatically detected)
	BufferDrawerData particleDrawData{};
	particleDrawData.framesInFlightBuffers = mUniformDescriptorManager.getPipelineDescriptorSSBO();
	particleDrawData.numVertices = PARTICLE_COUNT;
	particleDrawData.offset = 0;
	mParticleDrawer.setBufferDrawData(particleDrawData);

	PipelineDependencyInfo depInfo{};
	depInfo.dependsOnPipeline = &mComputePipelineStorageList[0];
	mGraphicsPipelineStorageList[1].setDependencyInfo(depInfo);


	mHouseGameObject = GameObject(mGraphicsPipelineStorageList[0].getBaseMaterial(), &mHouseMesh);


	//Configure drawable data for scenes
	std::vector<DrawableData> scene1DrawablesData = 
	{
		//Adds game object as a drawable to render
		{ mHouseGameObject.getMesh(), mHouseGameObject.getMaterial()->pipelineForMaterial, mHouseGameObject.getMaterial()},
	};

	std::vector<DrawableData> scene2DrawablesData =
	{
		//Compute pass for computing particles, followed by graphics pass for rendering particles (ordered when building render tree
		// from established dependency on graphics pass depending on compute pass)
		{ nullptr, &mComputePipelineStorageList[0], nullptr},
		{ &mParticleDrawer, &mGraphicsPipelineStorageList[1], mGraphicsPipelineStorageList[1].getBaseMaterial()},
	};

	mScenes.push_back({ MODEL, scene1DrawablesData, "Render Model", true });
	mScenes.push_back({ PARTICLES, scene2DrawablesData, "Render Particles", false });

	//Loads current scene by building the render graph tree with that scene's drawables
	mSelectedScene = mScenes[mCurrScene];
	mActiveRenderGraph.buildRenderTree(mScenes[mCurrScene].sceneGameObjects);
	

	//Fills initial particle data
	std::vector<Particle2D> particles(PARTICLE_COUNT);

	std::default_random_engine rngEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rngRange(0.0f, 1.0f);

	for (int i = 0; i < PARTICLE_COUNT; i++)
	{
		float r = 0.25 * sqrt(rngRange(rngEngine));
		float theta = rngRange(rngEngine) * 2.0 * 3.14159f;
		float x = r * cos(theta) * (mSwapChainImageExtent.height / (float)mSwapChainImageExtent.width);
		float y = r * sin(theta);
		particles[i].position = glm::vec2(x, y);
		particles[i].velocity = glm::normalize(particles[i].position) * 0.00025f * (rngRange(rngEngine) + 0.3f);
		particles[i].color = glm::vec3(rngRange(rngEngine), rngRange(rngEngine), rngRange(rngEngine));
	}

	mUniformDescriptorManager.addDataToSSBOs(mLogicalDevice, mPhysicalDevice, particles.data(),
		mComputePipelineStorageList[0].getPipelineBufferDescriptor(2));

	return true;
}

bool VulkanManager::cleanupVulkan()
{
	//Waits for rendering semaphores to finish
	vkDeviceWaitIdle(mLogicalDevice);

	int numSwapChainImages = mSwapChainImages.size();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(mLogicalDevice, mImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(mLogicalDevice, mWhileRenderingFences[i], nullptr);
	}

	for (size_t i = 0; i < numSwapChainImages; i++)
	{
		vkDestroySemaphore(mLogicalDevice, mRenderFinishedSemaphores[i], nullptr);
	}

	vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);

	cleanupSwapChain();

	vkDestroySampler(mLogicalDevice, mTextureSampler, nullptr);
	vkDestroyImageView(mLogicalDevice, mTextureImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mTextureImage, nullptr);
	vkFreeMemory(mLogicalDevice, mTextureMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mDepthImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mDepthImage, nullptr);
	vkFreeMemory(mLogicalDevice, mDepthMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mMSAA_ColorImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mMSAA_ColorImage, nullptr);
	vkFreeMemory(mLogicalDevice, mMSAA_ColorhMemory, nullptr);

	mUniformDescriptorManager.cleanup(mLogicalDevice);
	mHouseMesh.cleanupBuffers(mLogicalDevice);

	for (Pipeline& pipeline : mGraphicsPipelineStorageList)
	{
		pipeline.cleanupPipeline(mLogicalDevice);
	}

	for (Pipeline& pipeline : mComputePipelineStorageList)
	{
		pipeline.cleanupPipeline(mLogicalDevice);
	}

	vkDestroyDevice(mLogicalDevice, nullptr);

	if (enableValidationLayers) 
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessanger, nullptr);
	}

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);

	std::cout << "\nDeinitializing Vulkan" << std::endl;

	return false;
}

bool VulkanManager::drawFrame(GLFWwindow* window, float dt, bool* needToReloadGUI_Flag)
{
	VkSubmitInfo submitInfo{};


	//Updating of global uniforms
	mDtUniformObject.dt = dt;

	static std::chrono::system_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::system_clock::time_point currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(startTime - currentTime).count();

	mMVP_UniformObject.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	mMVP_UniformObject.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	mMVP_UniformObject.proj = glm::perspective(glm::radians(45.0f), mSwapChainImageExtent.width / (float)mSwapChainImageExtent.height, 0.1f, 10.0f);
	mMVP_UniformObject.proj[1][1] *= -1;

	//CPU waits until fence has been signaled by GPU (rendering done from last iteration of this frame in flight's index)
	vkWaitForFences(mLogicalDevice, 1, &mWhileRenderingFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(mLogicalDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], 
		VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//TODO: Maybe just store the window pointer as a member to avoid func parameter here
		recreateSwapChain(window);
		return true;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		std::cout << "Failed to aquire next swapchain image..." << std::endl;
		return false;
	}

	//Resets fence only if image has been aquired
	vkResetFences(mLogicalDevice, 1, &mWhileRenderingFences[mCurrentFrame]);
	vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);

	//Begins command buffer recording for rendering scene (TODO: Split up into multiple command buffers probobly,
	// depending on being a graphics or compute pipeline perhaps)
	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(mCommandBuffers[mCurrentFrame], &beginCommandBuffInfo) != VK_SUCCESS)
	{
		std::cout << "\nFailed to begin recording command buffer..." << std::endl;
		return false;
	}

	//Loops though all active pipelines that are used by the drawables in the active scene
	for (Pipeline* pipelineToRender : *mActiveRenderGraph.getOrderedActivePipelines())
	{
		//Updates the uniform buffer data of the current pipeline by copying up to date data into buffers
		mUniformDescriptorManager.updatePipelineSpecificUniformBuffer(pipelineToRender, mCurrentFrame);

		//Binds next active pipeline
		pipelineToRender->bindPipeline(mCommandBuffers[mCurrentFrame]);

		//Binds pipeline specific descriptor sets
		mUniformDescriptorManager.bindPipelineSpecificDescriptorSet(mCommandBuffers[mCurrentFrame], pipelineToRender,
			mCurrentFrame);

		//Injects memory barriers for next pipeline if they exist (for example if a graphics pipeline renders the result of a 
		// compute pipeline)
		handleInjectPipelineMemoryBarriers(mCommandBuffers[mCurrentFrame], pipelineToRender);

		//Loops through all active materials being used by game objects in active scene
		for (const std::pair<const Material*, std::vector<const Drawable*>>& materialsToRender :
			mActiveRenderGraph.getRenderTree()[pipelineToRender])
		{
			const Material* material = materialsToRender.first == nullptr ? pipelineToRender->getBaseMaterial() :
				materialsToRender.first;

			//Binds material specific descriptor sets for all materials that inherit from a core pipeline (binds nothing
			// if no descriptos are defined for the pipelines material, often the case for compute pipelines)
			mUniformDescriptorManager.bindMaterialSpecificDescriptorSet(mCommandBuffers[mCurrentFrame], material, 
				mCurrentFrame);

			//Executes all drawable/executables (mesh rendering or compute dispatches) by material (or by pipeline for compute)
			for (const Drawable* currDrawable : materialsToRender.second)
			{
				pipelineToRender->recordPipelineCommands(mCommandBuffers[mCurrentFrame], currDrawable,
					mSwapChainImages[imageIndex], mSwapChainImageViews[imageIndex], mCurrentFrame, fpCmdBeginRenderingKHR,
					fpCmdEndRenderingKHR);
			}
		}
	}

	//Renders ImGui
	renderGUI_DynamicRender(mCommandBuffers[mCurrentFrame], imageIndex);

	//Transitions swap chain image to the correct formats, accesses, and stages for presenting
	transitionImageLayout(mCommandBuffers[mCurrentFrame], mSwapChainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, 1, fpCmdPipelineBarrier2);

	if (vkEndCommandBuffer(mCommandBuffers[mCurrentFrame]) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end command buffer recording..." << std::endl;
		return false;
	}

	//Only adds the compute shader wait semephore if actually rendering the particles which depend on the compute shader
	std::vector<VkSemaphore> waitSemaphores = { mImageAvailableSemaphores[mCurrentFrame]};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[imageIndex];

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mWhileRenderingFences[mCurrentFrame]) != VK_SUCCESS)
	{
		std::cout << "Failed to submit command buffer to graphics queue..." << std::endl;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[imageIndex];

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebuffersResized)
	{
		//TODO: Maybe just store the window pointer as a member to avoid func parameter here
		recreateSwapChain(window);
		mFramebuffersResized = false;
		return true;
	}
	else if (result != VK_SUCCESS)
	{
		std::cout << "Failed to present swap chain image..." << std::endl;
		return false;
	}

	handlePipelineChanges(window, needToReloadGUI_Flag);

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return true;
}

bool VulkanManager::createInstance()
{
	if (enableValidationLayers && !hasValidationLayerSupport())
	{
		std::cout << "Validation layers requested but are not avalable..." << std::endl;
		return false;
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "VKRen";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions();

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
		createInfo.ppEnabledLayerNames = mValidationLayers.data();

		populateDebugMessangerInfo(debugMessengerInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		std::cout << "ERROR: Failed to create Vulkan instance..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::hasValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* currLayer : mValidationLayers)
	{
		bool layerFound = false;

		for (const VkLayerProperties& availableLayer : availableLayers) 
		{
			if (strcmp(availableLayer.layerName, currLayer)) 
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanManager::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanManager::setUpDebugMessenger()
{
	if (!enableValidationLayers) return false;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	populateDebugMessangerInfo(debugMessengerInfo);

	if (CreateDebugUtilsMessengerEXT(mInstance, &debugMessengerInfo, nullptr, &mDebugMessanger) != VK_SUCCESS)
	{
		std::cout << "Failed to set up debug messages." << std::endl;
		return false;
	}

	return true;
}

void VulkanManager::populateDebugMessangerInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo)
{
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debugCallback;
	debugMessengerInfo.pUserData = nullptr;
}

VkResult VulkanManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAlloator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//Makes function pointer to the create debug utils messenger function
	VkResult(*func)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*,
		const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*) =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAlloator, pDebugMessenger);
	}
	else {
		std::cout << "\nFailed to find vkCreateDebugUtilsMessengerEXT..." << std::endl;
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	void (*func)(VkInstance, VkDebugUtilsMessengerEXT,
		const VkAllocationCallbacks*) = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanManager::registerExtensionFunctions(VkInstance instance)
{
	fpCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
	fpCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
	fpCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2KHR)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR");
}

bool VulkanManager::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	for (VkPhysicalDevice currDevice : devices)
	{
		if (deviceIsSuitable(currDevice))
		{
			mPhysicalDevice = currDevice;
			mMSAA_Samples = getMaxUsableSampleCount();
			return true;
		}
	}

	std::cout << "\nFailed to find suitable physical device..." << std::endl;
	return false;
}

bool VulkanManager::deviceIsSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	bool supportsExtensions = supportsDeviceExtensions(device);

	bool swapChainAdequite = false;
	if (supportsExtensions)
	{
		SwapChainSupportDetails swapChainDetails = querySwapChainSupport(device);
		swapChainAdequite = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
	}

	return deviceProperties.apiVersion >= VK_API_VERSION_1_3 
		&& findSuitableQueueFamilies(device).containsAllFamilies()
		&& supportsExtensions 
		&& swapChainAdequite
		&& supportsDeviceFeatures(device);
}

QueueFamiliesIndexStore VulkanManager::findSuitableQueueFamilies(VkPhysicalDevice device)
{
	QueueFamiliesIndexStore queueIndexInfo{};
	queueIndexInfo.graphicsFamalyIndex = -1;
	queueIndexInfo.presentFamalyIndex = -1;

	uint32_t queuePropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueProperties(queuePropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, queueProperties.data());

	int i = 0;
	//Checks for graphics support
	for (VkQueueFamilyProperties queueProperty : queueProperties)
	{
		//Not using an asyncronous compute queue
		if ((queueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueProperty.queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			queueIndexInfo.graphicsFamalyIndex = i;
			queueIndexInfo.computeFamalyIndex = i;
		}

		VkBool32 presentSupported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupported);

		if (presentSupported)
		{
			queueIndexInfo.presentFamalyIndex = i;
		}

		if (queueIndexInfo.containsAllFamilies())
		{
			break;
		}

		i++;
	}

	return queueIndexInfo;
}

bool VulkanManager::supportsDeviceExtensions(VkPhysicalDevice device)
{
	uint32_t queueExtensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &queueExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> avalableQueueExtensions(queueExtensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &queueExtensionsCount, avalableQueueExtensions.data());

	std::set<std::string> requiredExtensionNames(mRequiredDeviceExtension.begin(), mRequiredDeviceExtension.end());

	//Checks for extension support by removing extension from required extensions list when one is found
	for (const VkExtensionProperties& queueExtension : avalableQueueExtensions)
	{
		requiredExtensionNames.erase(queueExtension.extensionName);
	}

	return requiredExtensionNames.empty();
}

bool VulkanManager::supportsDeviceFeatures(VkPhysicalDevice device)
{
	//Checks for feature support
	VkPhysicalDeviceFeatures2 deviceFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;
	deviceVulkan13Features.dynamicRendering = VK_TRUE;
	deviceVulkan13Features.synchronization2 = VK_TRUE;

	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;

	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

	return deviceVulkan13Features.synchronization2 && deviceVulkan13Features.dynamicRendering && 
		deviceExtendedStateFeatures.extendedDynamicState && deviceFeatures.features.samplerAnisotropy;
}

VkFormat VulkanManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Could not find supported format...");
}

VkFormat VulkanManager::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanManager::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool VulkanManager::createLogicalDevice()
{
	//Can replace this with the previously found queue index info when determining queue family suitability
	QueueFamiliesIndexStore queueFamilyIndeciesInfo = findSuitableQueueFamilies(mPhysicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilyIndecies = queueFamilyIndeciesInfo.getVectorOfIndecies();

	float queuePriority = 1.0;

	for (uint32_t queueIndex : queueFamilyIndecies)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceBaseFeatures{};
	deviceBaseFeatures.logicOp = true;
	deviceBaseFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceAdditFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;
	deviceVulkan13Features.dynamicRendering = VK_TRUE;
	deviceVulkan13Features.synchronization2 = VK_TRUE;

	deviceAdditFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceAdditFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;
	deviceAdditFeatures.features = deviceBaseFeatures;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	//deviceCreateInfo.pEnabledFeatures = &deviceBaseFeatures; //This is null because we ask for additional features in pNext
	deviceCreateInfo.pNext = (VkPhysicalDeviceFeatures2*)&deviceAdditFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtension.data();

	////Device Layers have never worked since Vulkan 1.0 and only Instance Layers should be used instead: 
	// https://docs.vulkan.org/spec/latest/appendices/legacy.html#legacy-devicelayers
	/*if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = mValidationLayers.data();
	}
	else*/
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create logical device..." << std::endl;
		return false;
	}

	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.graphicsFamalyIndex, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.computeFamalyIndex, 0, &mComputeQueue);
	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.presentFamalyIndex, 0, &mPresentQueue);

	return true;
}

bool VulkanManager::createSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create window surface..." << std::endl;
		return false;
	}

	return true;
}

SwapChainSupportDetails VulkanManager::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails swapChainDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &swapChainDetails.capabilities);

	uint32_t formatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatsCount, nullptr);

	if (formatsCount != 0)
	{
		swapChainDetails.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatsCount, swapChainDetails.formats.data());
	}

	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModesCount, nullptr);

	if (presentModesCount != 0)
	{
		swapChainDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModesCount, swapChainDetails.presentModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR VulkanManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats)
{
	for (const VkSurfaceFormatKHR& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	for (const VkPresentModeKHR& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D extents = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	extents.width = std::max(std::min(extents.width, capabilities.maxImageExtent.width), capabilities.minImageExtent.width);
	extents.height = std::max(std::min(extents.height, capabilities.maxImageExtent.height), capabilities.minImageExtent.height);

	return extents;
}

bool VulkanManager::createSwapChain(GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR swapFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
	VkPresentModeKHR swapPresentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
	VkExtent2D swapExtents = chooseSwapExtent(swapChainSupportDetails.capabilities, window);

	uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainSupportDetails.capabilities.maxImageCount;
	}

	imageCount = swapChainSupportDetails.capabilities.minImageCount;

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = mSurface;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = swapFormat.format;
	swapChainCreateInfo.imageColorSpace = swapFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapExtents;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamiliesIndexStore queueFamiliesInfo = findSuitableQueueFamilies(mPhysicalDevice);
	uint32_t indeciesArr[] = {
		static_cast<uint32_t>(queueFamiliesInfo.graphicsFamalyIndex),
		static_cast<uint32_t>(queueFamiliesInfo.presentFamalyIndex)
	};

	if (queueFamiliesInfo.graphicsFamalyIndex != queueFamiliesInfo.presentFamalyIndex)
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = indeciesArr;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = swapPresentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mLogicalDevice, &swapChainCreateInfo, nullptr, &mSwapChain) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create swap chain..." << std::endl;
		return false;
	}

	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapChain, &imageCount, mSwapChainImages.data());

	mSwapChainImageFormat = swapFormat.format;
	mSwapChainImageExtent = swapExtents;

	return true;
}

bool VulkanManager::recreateSwapChain(GLFWwindow* window)
{
	int width, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	
	//Stalls until window is valid/maximized
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(mLogicalDevice);

	cleanupSwapChain();

	vkDestroyImageView(mLogicalDevice, mDepthImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mDepthImage, nullptr);
	vkFreeMemory(mLogicalDevice, mDepthMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mMSAA_ColorImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mMSAA_ColorImage, nullptr);
	vkFreeMemory(mLogicalDevice, mMSAA_ColorhMemory, nullptr);

	createSwapChain(window);
	createSwapChainImageViews();
	createMSAA_ColorResources();
	createDepthResources();

	for (GraphicsPipeline& graphicsPipeline : mGraphicsPipelineStorageList)
	{
		graphicsPipeline.updateRenderExtents(mSwapChainImageExtent);

		if (!mUsingDynamicRendering)
		{
			graphicsPipeline.createFramebuffers(mLogicalDevice, mSwapChainImageViews, mDepthImageView, mMSAA_ColorImageView,
				mSwapChainImageExtent);
		}
	}
	
	return true;
}

void VulkanManager::cleanupSwapChain()
{
	for (GraphicsPipeline& graphicsPipeline : mGraphicsPipelineStorageList)
	{
		graphicsPipeline.cleanupFrambuffers(mLogicalDevice);
	}

	for (VkImageView imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mLogicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(mLogicalDevice, mSwapChain, nullptr);
}

bool VulkanManager::createSwapChainImageViews()
{
	int imageCount = mSwapChainImages.size();
	mSwapChainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; i++)
	{
		if (!createImageView(mLogicalDevice, mSwapChainImages[i], &mSwapChainImageViews[i], mSwapChainImageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, 1))
		{
			std::cout << "\nFailed to create swap chain image view..." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanManager::createDepthResources()
{
	mDepthFormat = findDepthFormat();

	if (!createImage(mLogicalDevice, mPhysicalDevice, mSwapChainImageExtent.width, mSwapChainImageExtent.height, 1, mMSAA_Samples,
		mDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mDepthImage, mDepthMemory)) {
		return false;
	}

	if (!createImageView(mLogicalDevice, mDepthImage, &mDepthImageView, mDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1)) { return false; }

	return true;
}

bool VulkanManager::createMSAA_ColorResources()
{
	if (!createImage(mLogicalDevice, mPhysicalDevice, mSwapChainImageExtent.width, mSwapChainImageExtent.height, 1,
		mMSAA_Samples, mSwapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mMSAA_ColorImage, mMSAA_ColorhMemory))
	{
		return false;
	}

	if (!createImageView(mLogicalDevice, mMSAA_ColorImage, &mMSAA_ColorImageView, mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1))
	{
		return false;
	}

	return true;
}

bool VulkanManager::createCommandPool()
{
	QueueFamiliesIndexStore queueFamilies = findSuitableQueueFamilies(mPhysicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamalyIndex;

	if (vkCreateCommandPool(mLogicalDevice, &poolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create command pool..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::createCommandBuffers()
{
	mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo commandBufAllocateInfo{};
	commandBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufAllocateInfo.commandPool = mCommandPool;
	commandBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufAllocateInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(mLogicalDevice, &commandBufAllocateInfo, mCommandBuffers.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate graphics command buffers..." << std::endl;
		return false;
	}

	return true;
}

void VulkanManager::handlePipelineChanges(GLFWwindow* window, bool* needToReloadGUI_Flag)
{
	if (mSwapScenesTriggered)
	{
		//Waits for device to finish up before switching scene
		vkDeviceWaitIdle(mLogicalDevice);

		for (const DrawableData& drawableData : mScenes[mCurrScene].sceneGameObjects)
		{
			mActiveRenderGraph.removeDrawableFromRenderTree(drawableData);
		}

		mScenes[mCurrScene].selected = false;

		mCurrScene++;
		if (mCurrScene == mScenes.size()) { mCurrScene = 0; }

		mActiveRenderGraph.buildRenderTree(mScenes[mCurrScene].sceneGameObjects);
		mScenes[mCurrScene].selected = true;

		mSwapScenesTriggered = false;
	}

	if (mSwitchingRenderMethod)
	{
		//Waits for device to finish up before recreating pipelines
		vkDeviceWaitIdle(mLogicalDevice);

		for (GraphicsPipeline& graphicsPipeline : mGraphicsPipelineStorageList)
		{
			graphicsPipeline.cleanupFrambuffers(mLogicalDevice);
			graphicsPipeline.cleanupRenderPass(mLogicalDevice);
			vkDestroyPipeline(mLogicalDevice, graphicsPipeline.getPipeline(), nullptr);
			vkDestroyPipelineLayout(mLogicalDevice, graphicsPipeline.getPipelineLayout(), nullptr);
			graphicsPipeline.setDynamicRenderingEnabled(mUsingDynamicRendering);
		}

		//TODO: Store pipeline create data in the class and then just call create again instead of having to pass config values again
		ConfigurablePipelineValues configValues{};
		configValues.samples = mMSAA_Samples;
		configValues.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configValues.depthWriteEnabled = true;
		configValues.targetDepthImage = &mDepthImage;
		configValues.targetDepthImageView = &mDepthImageView;
		configValues.targetMSAA_Image = &mMSAA_ColorImage;
		configValues.targetMSAA_ImageView = &mMSAA_ColorImageView;

		char* vertShader = "../Assets/Shaders/ByteEncoded/RenderModel_VS.spv";
		char* fragShader = "../Assets/Shaders/ByteEncoded/RenderModel_FS.spv";

		VertexInputData vertexInputInfo = mHouseMesh.getVertexInputData();

		mGraphicsPipelineStorageList[0].createPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat, configValues,
			vertShader, fragShader, vertexInputInfo, mSwapChainImageViews, mUsingDynamicRendering);

		configValues = {};
		configValues.samples = mMSAA_Samples;
		configValues.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		configValues.depthWriteEnabled = false;
		configValues.targetDepthImage = &mDepthImage;
		configValues.targetDepthImageView = &mDepthImageView;
		configValues.targetMSAA_Image = &mMSAA_ColorImage;
		configValues.targetMSAA_ImageView = &mMSAA_ColorImageView;

		vertShader = "../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv";
		fragShader = "../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv";

		vertexInputInfo = Particle2D::getParticleInputData();

		mGraphicsPipelineStorageList[1].createPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat, configValues,
			vertShader, fragShader, vertexInputInfo, mSwapChainImageViews, mUsingDynamicRendering);

		mSwitchingRenderMethod = false;

		if (needToReloadGUI_Flag != nullptr)
		{
			*needToReloadGUI_Flag = true;
		}
	}
}

void VulkanManager::handleInjectPipelineMemoryBarriers(VkCommandBuffer commandBuffer, Pipeline* sourcePipeline)
{
	const PipelineDependencyInfo* depInfo = sourcePipeline->getPipelineDependencyInfo();
	if (depInfo->dependsOnPipeline != nullptr)
	{
		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(depInfo->buffMemBarriers.size());
		dependencyInfo.pBufferMemoryBarriers = depInfo->buffMemBarriers.data();

		fpCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}
}

bool VulkanManager::createSyncObjects()
{
	int numSwapChainImages = mSwapChainImages.size();

	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mWhileRenderingFences.resize(MAX_FRAMES_IN_FLIGHT);

	//This is to stop unsafe reusage of semaphores: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
	mRenderFinishedSemaphores.resize(numSwapChainImages);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Makes it so first drawFrame does not block

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(mLogicalDevice, &fenceCreateInfo, nullptr, &mWhileRenderingFences[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create semaphores or fence..." << std::endl;
			return false;
		}
	}

	for (size_t i = 0; i < numSwapChainImages; i++)
	{
		if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create semaphores or fence..." << std::endl;
			return false;
		}
	}



	return true;
}

void VulkanManager::renderGUI_DynamicRender(VkCommandBuffer commandBuffer, int imageIndex)
{
	updateGUI();

	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginCommandBuffInfo.flags = 0;
	beginCommandBuffInfo.pInheritanceInfo = nullptr;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachmentInfo.clearValue = clearColor;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.imageView = mSwapChainImageViews[imageIndex];

	//Adds resolve image information if using MSAA
	if (mMSAA_Samples != VK_SAMPLE_COUNT_1_BIT)
	{
		colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.resolveImageView = mSwapChainImageViews[imageIndex];
		colorAttachmentInfo.imageView = mMSAA_ColorImageView;
	}

	//std::array<VkRenderingAttachmentInfoKHR, 1> colorAttachments = { colorAttachmentInfo };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = mSwapChainImageExtent;
	renderingInfo.layerCount = 1;

	fpCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE);

	fpCmdEndRenderingKHR(commandBuffer);
}

VkSampleCountFlagBits VulkanManager::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

	VkSampleCountFlags count = props.limits.framebufferColorSampleCounts & props.limits.sampledImageDepthSampleCounts;

	if (count & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (count & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (count & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (count & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (count & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (count & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanManager::markFramebuffersResized()
{
	mFramebuffersResized = true;
}

void VulkanManager::updateGUI()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Begin("DEBUG");

	if (ImGui::BeginCombo("Select Scene", mSelectedScene.name.c_str()))
	{
		for (SceneData scene : mScenes)
		{
			if (ImGui::Selectable(scene.name.c_str(), &scene.selected))
			{
				mSelectedScene = scene;
				mSwapScenesTriggered = true;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Checkbox("Using Dynamic Rendering", &mUsingDynamicRenderingForGUI))
	{
		mSwitchingRenderMethod = true;
	}

	ImGui::End();

	ImGui::Render();
}

void VulkanManager::renderGUI(VkCommandBuffer commandBuffer, int imageIndex)
{
	updateGUI();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE);
}
