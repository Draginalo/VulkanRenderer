#pragma once

#include "vulkan/vulkan.h"
#include "UniformObjects/UniformBufferDescriptor.h"
#include "UniformObjects/UniformImageDescriptor.h"
#include <vector>
#include <iostream>
#include <unordered_map>

class DescriptorSetsData {
public:
	void loadDescriptorSets(std::vector<UniformBufferDescriptor*> uniformBufferDescriptors,
		std::vector<UniformImageDescriptor*> uniformImageDescriptors, std::vector<UniformBufferDescriptor*> computeUniformBufferDescriptors,
		std::vector<UniformImageDescriptor*> computeUniformImageDescriptors, int maxFramesInFlight);

	std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings();
	std::vector<VkDescriptorSetLayoutBinding> getComputeLayoutBindings();
	std::vector<VkWriteDescriptorSet> getWriteDescriptorSets(VkDescriptorSet destSet,
		std::vector<VkBuffer> destUniformBuffers, std::vector<VkBuffer> destStorageBuffers, size_t currFrame);
	std::vector<VkWriteDescriptorSet> getComputeWriteDescriptorSets(VkDescriptorSet destSet,
		std::vector<VkBuffer> destUniformBuffers, std::vector<VkBuffer> destStorageBuffers, size_t currFrame);
	inline std::vector<UniformBufferDescriptor*> getUniformBufferDescriptors() { return mUniformBufferDescriptors; }
	inline std::vector<UniformBufferDescriptor*> getComputeUniformBufferDescriptors() { return mComputeUniformBufferDescriptors; }

	void updateBufferUniforms(void* destBuffer);
	void updateComputeBufferUniforms(void* destBuffer);

	inline VkDeviceSize getTotalCombinedUniformBufferSize() { return mTotalUniformBufferSize + mTotalComputeUniformBufferSize; }
	inline VkDeviceSize getTotalCombinedStorageBufferSize() { return mTotalStorageBufferSize + mTotalComputeStorageBufferSize; }
private:
	VkDescriptorPool mDescriptorPool;

	VkDeviceSize mTotalUniformBufferSize = 0;
	VkDeviceSize mTotalComputeUniformBufferSize = 0;

	VkDeviceSize mTotalStorageBufferSize = 0;
	VkDeviceSize mTotalComputeStorageBufferSize = 0;

	std::vector<UniformBufferDescriptor*> mUniformBufferDescriptors;
	std::vector<UniformImageDescriptor*> mUniformImageDescriptors;

	std::vector<UniformBufferDescriptor*> mComputeUniformBufferDescriptors;
	std::vector<UniformImageDescriptor*> mComputeUniformImageDescriptors;

	std::vector<std::unordered_map<VkBuffer, uint32_t>> mBufferToOffset = {};
};