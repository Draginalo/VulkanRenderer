#pragma once

#include "vulkan/vulkan.h"
#include <iostream>

enum DescriptorLevel {
	GLOBAL,
	PIPELINE_SPECIFIC,
	MATERIAL_SPECIFIC,
	OBJECT_SPECIFIC
};

class UniformDescriptor {
public:
	UniformDescriptor(DescriptorLevel descriptorLevel, uint32_t dstBinding = 0, 
		VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT, 
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) : mDescriptorLevel(descriptorLevel), mDstBinding(dstBinding),
		mStageFlags(stageFlags), mUniformType(uniformType) {};

	virtual inline VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() = 0;

	virtual inline VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet destSet) = 0;

	virtual inline void setDstBinding(uint32_t binding) { mDstBinding = binding; }
	virtual inline uint32_t getDstBinding() { return mDstBinding; }
	virtual inline void setStageFlags(VkShaderStageFlagBits stageFlags) { mStageFlags = stageFlags; }
	virtual inline VkShaderStageFlagBits getStageFlags() { return mStageFlags; }
	virtual inline void setUniformType(VkDescriptorType uniformType) { mUniformType = uniformType; }
	virtual inline const VkDescriptorType getUniformType() const { return mUniformType; }
	virtual inline const DescriptorLevel getDescriptorLevel() const { return mDescriptorLevel; }

	bool operator==(const UniformDescriptor& other) 
	{
		return other.mDstBinding == mDstBinding && other.mStageFlags == mStageFlags && other.mUniformType == mUniformType;
	}
protected:
	uint32_t mDstBinding;
	VkShaderStageFlagBits mStageFlags;
	VkDescriptorType mUniformType;
	DescriptorLevel mDescriptorLevel;
};