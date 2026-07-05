#include "UniformDescriptor.h"

UniformDescriptor::UniformDescriptor(DescriptorLevel descriptorLevel, uint32_t dstBinding, VkShaderStageFlagBits stageFlags, 
	VkDescriptorType uniformType) : mDstBinding(dstBinding), mStageFlags(stageFlags), mUniformType(uniformType), 
	mDescriptorLevel(descriptorLevel) {}

//Inline one liners
inline void UniformDescriptor::setDstBinding(uint32_t binding) { mDstBinding = binding; }
inline uint32_t UniformDescriptor::getDstBinding() const { return mDstBinding; }
inline void UniformDescriptor::setStageFlags(VkShaderStageFlagBits stageFlags) { mStageFlags = stageFlags; }
inline VkShaderStageFlagBits UniformDescriptor::getStageFlags() const { return mStageFlags; }
inline void UniformDescriptor::setUniformType(VkDescriptorType uniformType) { mUniformType = uniformType; }
inline VkDescriptorType UniformDescriptor::getUniformType() const { return mUniformType; }
inline DescriptorLevel UniformDescriptor::getDescriptorLevel() const { return mDescriptorLevel; }

bool UniformDescriptor::operator==(const UniformDescriptor& other)
{ return other.mDstBinding == mDstBinding && other.mStageFlags == mStageFlags && other.mUniformType == mUniformType; }
