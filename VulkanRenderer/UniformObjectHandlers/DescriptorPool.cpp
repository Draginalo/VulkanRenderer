#include "DescriptorPool.h"

bool DescriptorPool::createDescriptorPool(VkDevice logicalDevice, DescriptorPoolCreateData createData)
{
	std::vector<VkDescriptorPoolSize> poolSizes{};
	
	if (createData.uniformBufferCount > 0) { 
		poolSizes.push_back({
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			createData.maxFramesInFlight * createData.uniformBufferCount
		}); 
	}

	if (createData.combinedImageSamplerCount > 0) {
		poolSizes.push_back({
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			createData.maxFramesInFlight * createData.combinedImageSamplerCount
			});
	}

	if (createData.storageBufferCount > 0) {
		poolSizes.push_back({
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			createData.maxFramesInFlight * createData.storageBufferCount
			});
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = createData.maxFramesInFlight * createData.maxDescriptorSets; // *2 for descriptor sets from graphics and compute pipelines

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create descriptor pool..." << std::endl;
		return false;
	}

	return true;
}

void DescriptorPool::cleanup(VkDevice logicalDevice)
{
	vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
}
