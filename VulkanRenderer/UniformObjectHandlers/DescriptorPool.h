#pragma once

#include "vulkan/vulkan.h"
#include <vector>
#include <iostream>

struct DescriptorPoolCreateData {
	uint32_t uniformBufferCount;
	uint32_t combinedImageSamplerCount;
	uint32_t storageBufferCount;
	uint32_t maxFramesInFlight;
	uint32_t maxDescriptorSets;
};

class DescriptorPool {
public:
	bool createDescriptorPool(VkDevice logicalDevice, DescriptorPoolCreateData createData);
	void cleanup(VkDevice logicalDevice);

	inline VkDescriptorPool getDescriptorPool() { return mDescriptorPool; }
private:
	VkDescriptorPool mDescriptorPool;
};