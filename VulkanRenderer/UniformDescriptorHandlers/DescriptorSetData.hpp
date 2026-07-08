#pragma once

#include <vulkan/vulkan.h>
#include "UniformDescriptors/UniformBufferDescriptor.hpp"
#include "UniformDescriptors/UniformImageDescriptor.hpp"
#include <vector>
#include <iostream>
#include <unordered_map>

struct BufferData {
	std::vector<VkBuffer> buffers;
	uint32_t currentlyAllocatedSize = 0;

	//Explicit padding for x64 since struct size is not divisable by 8 in x64
	#if defined(_WIN64) || defined(__x86_64__)
		uint8_t padding[4];
	#endif
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
	VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

	VkDeviceSize mTotalUniformBufferSize = 0;
	VkDeviceSize mTotalStorageBufferSize = 0;

	std::vector<UniformBufferDescriptor> mUniformBufferDescriptors;
	std::vector<UniformImageDescriptor> mUniformImageDescriptors;
	std::vector<VkDescriptorSet> mDescriptorSets;

	uint32_t mTotalDescriptorsForMaterial = 0;

	//Explicit padding for compiler warning (4 bits of padding EXCEPT for x86 release mode (when size of a vector 
	// is only devisible by 4 and not divisable by 8))
	#if ((defined(_M_IX86) || defined(__i386__)) && !defined(_DEBUG))
		uint8_t padding[8] = {};
	#else
		uint8_t padding[4] = {};
	#endif
};