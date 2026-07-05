#pragma once

#include "vulkan/vulkan.h"
#include "UniformDescriptors/UniformBufferDescriptor.h"
#include "UniformDescriptors/UniformImageDescriptor.h"
#include <vector>
#include <iostream>
#include <unordered_map>

struct BufferData {
	std::vector<VkBuffer> buffers;
	uint32_t currentlyAllocatedSize = 0;

	//Padding for compiler warning
	uint8_t padding[4];
};

class DescriptorSetData {
public:
	void loadDescriptors(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors);

	bool createDescriptorSetLayout(VkDevice logicalDevice);
	bool createDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool, BufferData* destUniformBuffers, 
		BufferData* destStorageBuffers, uint32_t maxFramesInFlight);

	std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings();
	std::vector<VkWriteDescriptorSet> getWriteDescriptorSets(VkDescriptorSet destSet, BufferData* destUniformBuffers, 
		BufferData* destStorageBuffers, uint32_t currFrame);
	const std::vector<UniformBufferDescriptor>* getUniformBufferDescriptors() const;
	const std::vector<UniformImageDescriptor>* getUniformImageDescriptors() const;

	//Dev remove this 
	std::vector<UniformBufferDescriptor>* getUniformBufferDescriptorsRef();

	void updateBufferUniforms(void* destBuffer) const;

	VkDeviceSize getTotalUniformBufferSize() const;
	VkDeviceSize getTotalStorageBufferSize() const;

	const VkDescriptorSet* getDescriptorSet(uint32_t currFrame) const;
	VkDescriptorSetLayout* getDescriptorSetLayout();

	uint32_t getTotalDescriptorsForMaterial() const;

	void cleanup(VkDevice logicalDevice) const;
private:
	std::vector<UniformBufferDescriptor> mUniformBufferDescriptors;
	std::vector<UniformImageDescriptor> mUniformImageDescriptors;
	std::vector<VkDescriptorSet> mDescriptorSets;

	VkDescriptorSetLayout mDescriptorSetLayout = nullptr;

	VkDeviceSize mTotalUniformBufferSize = 0;
	VkDeviceSize mTotalStorageBufferSize = 0;

	uint32_t mTotalDescriptorsForMaterial = 0;

	//Padding for compiler warning
	uint8_t padding[4] = {};
};