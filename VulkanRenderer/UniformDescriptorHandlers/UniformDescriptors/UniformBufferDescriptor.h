#pragma once

#include "UniformDescriptor.h"
#include <iostream>


class UniformBufferDescriptor : public UniformDescriptor {
public:
	UniformBufferDescriptor(DescriptorLevel descriptorLevel = GLOBAL, uint32_t dstBinding = 0, 
		VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		VkDescriptorType uniformType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t dataSize = 0, void* dataPointer = nullptr, 
		bool sourceFromPastFrame = false);

	void setBufferInfo(VkDescriptorBufferInfo bufferInfo);
	void setDataPointer(void* pData);
	const void* getDataPointer() const;
	void setSourceFromPastFrame(bool sourceFromPastFrame);
	bool getSourceFromPastFrame() const;
	VkDeviceSize getDataSize() const;
	VkDeviceSize getOffset() const;
	VkBuffer getCurrBuffer() const;

	VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override {
		VkDescriptorSetLayoutBinding layout{};
		layout.binding = mDstBinding;
		layout.descriptorCount = 1;
		layout.descriptorType = mUniformType;
		layout.pImmutableSamplers = nullptr;
		layout.stageFlags = mStageFlags;

		return layout;
	}

	VkWriteDescriptorSet getWriteDescriptorSet(VkDescriptorSet destSet) override {
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

	void* mDataPointer = nullptr;
	bool mSourceFromPastFrame = false;

	//Explicit padding for compiler warning, since there is 3 bytes of padding EXCEPT when this class' size is not 
	// divisable by 8 in x86
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[3] = {};
	#else
		uint8_t padding[7] = {};
	#endif
};