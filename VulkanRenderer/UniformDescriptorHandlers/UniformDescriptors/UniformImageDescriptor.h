#pragma once

#include "UniformDescriptor.h"
#include <iostream>

class UniformImageDescriptor : public UniformDescriptor {
public:
	UniformImageDescriptor(DescriptorLevel descriptorLevel = GLOBAL, uint32_t dstBinding = 0, 
		VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkDescriptorImageInfo imageInfo = {});
	UniformImageDescriptor(const UniformImageDescriptor&) = default;
	UniformImageDescriptor& operator=(const UniformImageDescriptor& other) = default;

	~UniformImageDescriptor() = default;

	void setImageInfo(VkDescriptorImageInfo imageInfo);

	VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override {
		VkDescriptorSetLayoutBinding layout{};
		layout.binding = mDstBinding;
		layout.descriptorCount = 1;
		layout.descriptorType = mUniformType;
		layout.pImmutableSamplers = nullptr;
		layout.stageFlags = mStageFlags;

		return layout;
	}

	VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet targetSet) override {
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = targetSet;
		descriptorWrite.dstBinding = mDstBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = mUniformType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &mImageInfo;

		return descriptorWrite;
	}
private:
	VkDescriptorImageInfo mImageInfo;
};