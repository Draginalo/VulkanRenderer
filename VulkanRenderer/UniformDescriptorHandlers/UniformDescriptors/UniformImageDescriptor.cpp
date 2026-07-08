#include "UniformImageDescriptor.hpp"

UniformImageDescriptor::UniformImageDescriptor(DescriptorLevel descriptorLevel, uint32_t dstBinding,
	VkShaderStageFlagBits stageFlags, VkDescriptorType uniformType, VkDescriptorImageInfo imageInfo) :
	UniformDescriptor(descriptorLevel, dstBinding, stageFlags, uniformType), mImageInfo(imageInfo) { }

void UniformImageDescriptor::setImageInfo(VkDescriptorImageInfo imageInfo) { mImageInfo = imageInfo; }