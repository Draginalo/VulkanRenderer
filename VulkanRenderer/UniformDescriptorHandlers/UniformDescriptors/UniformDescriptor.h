#pragma once

#include <vulkan/vulkan.h>
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

	virtual void setDstBinding(uint32_t binding);
	virtual uint32_t getDstBinding() const;
	virtual void setStageFlags(VkShaderStageFlagBits stageFlags);
	virtual VkShaderStageFlagBits getStageFlags() const;
	virtual void setUniformType(VkDescriptorType uniformType);
	virtual VkDescriptorType getUniformType() const;
	virtual DescriptorLevel getDescriptorLevel() const;

	bool operator==(const UniformDescriptor& other);
protected:
	uint32_t mDstBinding;
	VkShaderStageFlagBits mStageFlags;
	VkDescriptorType mUniformType;
	DescriptorLevel mDescriptorLevel;

	//Explicit padding for compiler warning, since this class' size is not divisable by 8 in x86
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[4] = {};
	#endif
};