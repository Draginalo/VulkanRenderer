#include "UniformBufferDescriptor.h"

UniformBufferDescriptor::UniformBufferDescriptor(DescriptorLevel descriptorLevel, uint32_t dstBinding, 
	VkShaderStageFlagBits stageFlags, VkDescriptorType uniformType, uint32_t dataSize, void* dataPointer, 
	bool sourceFromPastFrame) : UniformDescriptor(descriptorLevel, dstBinding, stageFlags, uniformType),
	mBufferInfo({ nullptr, 0, dataSize }), mDataPointer(dataPointer), mSourceFromPastFrame(sourceFromPastFrame) { }

//Inline one liners
void UniformBufferDescriptor::setBufferInfo(VkDescriptorBufferInfo bufferInfo) { mBufferInfo = bufferInfo; }
void UniformBufferDescriptor::setDataPointer(void* pData) { mDataPointer = pData; }
const void* UniformBufferDescriptor::getDataPointer() const { return mDataPointer; }
void UniformBufferDescriptor::setSourceFromPastFrame(bool sourceFromPastFrame)
{ mSourceFromPastFrame = sourceFromPastFrame; }
bool UniformBufferDescriptor::getSourceFromPastFrame() const { return mSourceFromPastFrame; }
VkDeviceSize UniformBufferDescriptor::getDataSize() const { return mBufferInfo.range; }
VkDeviceSize UniformBufferDescriptor::getOffset() const { return mBufferInfo.offset; }
VkBuffer UniformBufferDescriptor::getCurrBuffer() const { return mBufferInfo.buffer; }