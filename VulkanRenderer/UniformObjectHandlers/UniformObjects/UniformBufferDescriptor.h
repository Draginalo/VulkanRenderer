#pragma once

#include "UniformDescriptor.h"
#include <iostream>

class UniformBufferDescriptor : public UniformDescriptor {
public:
	UniformBufferDescriptor(uint32_t dstBinding = 0, VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t dataSize = 0, void* dataPointer = nullptr, 
		bool sourceFromPastFrame = false) : UniformDescriptor(dstBinding, stageFlags, uniformType), 
		mBufferInfo({nullptr, 0, dataSize}), mDataPointer(dataPointer), mSourceFromPastFrame(sourceFromPastFrame) {};

	inline void setBufferInfo(VkDescriptorBufferInfo bufferInfo) { mBufferInfo = bufferInfo; }
	inline void setDataPointer(void* pData) { mDataPointer = pData; }
	inline void* getDataPointer() { return mDataPointer; }
	inline void setSourceFromPastFrame(bool sourceFromPastFrame) { mSourceFromPastFrame = sourceFromPastFrame; }
	inline bool getSourceFromPastFrame() { return mSourceFromPastFrame; }
	inline VkDeviceSize getDataSize() { return mBufferInfo.range; }
	inline VkDeviceSize getOffset() { return mBufferInfo.offset; }

	inline VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override {
		VkDescriptorSetLayoutBinding layout{};
		layout.binding = mDstBinding;
		layout.descriptorCount = 1;
		layout.descriptorType = mUniformType;
		layout.pImmutableSamplers = nullptr;
		layout.stageFlags = mStageFlags;

		return layout;
	}

	inline VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet destSet) override {
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = destSet;
		descriptorWrite.dstBinding = mDstBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = mUniformType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &mBufferInfo;

		return descriptorWrite;
	}
private:
	VkDescriptorBufferInfo mBufferInfo;

	bool mSourceFromPastFrame = false;
	void* mDataPointer = nullptr;
};