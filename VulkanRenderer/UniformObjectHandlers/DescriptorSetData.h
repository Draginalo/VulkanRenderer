#pragma once

#include "vulkan/vulkan.h"
#include "UniformObjects/UniformBufferDescriptor.h"
#include "UniformObjects/UniformImageDescriptor.h"
#include <vector>
#include <iostream>
#include <unordered_map>

struct BufferData {
	std::vector<VkBuffer> buffers;
	uint32_t currentlyAllocatedSize = 0;
};

class DescriptorSetData {
public:
	void loadDescriptors(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor> uniformImageDescriptors, int maxFramesInFlight);

	bool createDescriptorSetLayout(VkDevice logicalDevice);
	bool createDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool, BufferData* destUniformBuffers, 
		BufferData* destStorageBuffers, int maxFramesInFlight);

	std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings();
	std::vector<VkWriteDescriptorSet> getWriteDescriptorSets(VkDescriptorSet destSet, BufferData* destUniformBuffers, 
		BufferData* destStorageBuffers, size_t currFrame);
	inline const std::vector<UniformBufferDescriptor>* getUniformBufferDescriptors() const { return &mUniformBufferDescriptors; }
	inline const std::vector<UniformImageDescriptor>* getUniformImageDescriptors() const { return &mUniformImageDescriptors; }

	//Dev remove this 
	inline std::vector<UniformBufferDescriptor>* getUniformBufferDescriptorsRef() { return &mUniformBufferDescriptors; }

	void updateBufferUniforms(void* destBuffer) const;

	inline const VkDeviceSize getTotalUniformBufferSize() const { return mTotalUniformBufferSize; }
	inline const VkDeviceSize getTotalStorageBufferSize() const { return mTotalStorageBufferSize; }

	inline const VkDescriptorSet* getDescriptorSet(int currFrame) const { return &mDescriptorSets[currFrame]; }
	inline VkDescriptorSetLayout* getDescriptorSetLayout() { return &mDescriptorSetLayout; }

	inline void cleanup(VkDevice logicalDevice) const { vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr); }
private:
	VkDescriptorSetLayout mDescriptorSetLayout = nullptr;
	std::vector<VkDescriptorSet> mDescriptorSets;

	VkDeviceSize mTotalUniformBufferSize = 0;
	VkDeviceSize mTotalStorageBufferSize = 0;

	std::vector<UniformBufferDescriptor> mUniformBufferDescriptors;
	std::vector<UniformImageDescriptor> mUniformImageDescriptors;
};