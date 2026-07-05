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
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	//Explicit class defenitions to avoid compiler warnings
	UniformDescriptor(const UniformDescriptor&) = default;
	UniformDescriptor& operator=(const UniformDescriptor& other) = default;

	virtual ~UniformDescriptor() = default;

	virtual inline VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() = 0;

	virtual inline VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet destSet) = 0;

	virtual inline void setDstBinding(uint32_t binding);
	virtual inline uint32_t getDstBinding() const;
	virtual inline void setStageFlags(VkShaderStageFlagBits stageFlags);
	virtual inline VkShaderStageFlagBits getStageFlags() const;
	virtual inline void setUniformType(VkDescriptorType uniformType);
	virtual inline VkDescriptorType getUniformType() const;
	virtual inline DescriptorLevel getDescriptorLevel() const;

	bool operator==(const UniformDescriptor& other);
protected:
	uint32_t mDstBinding;
	VkShaderStageFlagBits mStageFlags;
	VkDescriptorType mUniformType;
	DescriptorLevel mDescriptorLevel;
};