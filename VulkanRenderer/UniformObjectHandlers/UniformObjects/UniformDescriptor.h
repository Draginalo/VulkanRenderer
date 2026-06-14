#pragma once

#include "vulkan/vulkan.h"
#include <iostream>

class UniformDescriptor {
public:
	UniformDescriptor(uint32_t dstBinding = 0, VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT, 
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) : mDstBinding(dstBinding),
		mStageFlags(stageFlags), mUniformType(uniformType) {};

	virtual inline VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() = 0;

	virtual inline VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet destSet) = 0;

	virtual inline void setDstBinding(uint32_t binding) { mDstBinding = binding; }
	virtual inline uint32_t getDstBinding() { return mDstBinding; }
	virtual inline void setStageFlags(VkShaderStageFlagBits stageFlags) { mStageFlags = stageFlags; }
	virtual inline VkShaderStageFlagBits getStageFlags() { return mStageFlags; }
	virtual inline void setUniformType(VkDescriptorType uniformType) { mUniformType = uniformType; }
	virtual inline VkDescriptorType getUniformType() { return mUniformType; }
protected:
	uint32_t mDstBinding;
	VkShaderStageFlagBits mStageFlags;
	VkDescriptorType mUniformType;
};