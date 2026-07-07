#include "UniformDescriptor.h"

UniformDescriptor::UniformDescriptor(DescriptorLevel descriptorLevel, uint32_t dstBinding, VkShaderStageFlagBits stageFlags, 
	VkDescriptorType uniformType) : mDstBinding(dstBinding), mStageFlags(stageFlags), mUniformType(uniformType), 
	mDescriptorLevel(descriptorLevel) {}

//Inline one liners
void UniformDescriptor::setDstBinding(uint32_t binding) { mDstBinding = binding; }
uint32_t UniformDescriptor::getDstBinding() const { return mDstBinding; }
void UniformDescriptor::setStageFlags(VkShaderStageFlagBits stageFlags) { mStageFlags = stageFlags; }
VkShaderStageFlagBits UniformDescriptor::getStageFlags() const { return mStageFlags; }
void UniformDescriptor::setUniformType(VkDescriptorType uniformType) { mUniformType = uniformType; }
VkDescriptorType UniformDescriptor::getUniformType() const { return mUniformType; }
DescriptorLevel UniformDescriptor::getDescriptorLevel() const { return mDescriptorLevel; }

bool UniformDescriptor::operator==(const UniformDescriptor& other)
{ return other.mDstBinding == mDstBinding && other.mStageFlags == mStageFlags && other.mUniformType == mUniformType; }
