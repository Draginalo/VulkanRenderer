#include "DescriptorSetsData.h"

void DescriptorSetsData::loadDescriptorSets(std::vector<UniformBufferDescriptor*> uniformBufferDescriptors, 
	std::vector<UniformImageDescriptor*> uniformImageDescriptors, std::vector<UniformBufferDescriptor*> computeUniformBufferDescriptors,
	std::vector<UniformImageDescriptor*> computeUniformImageDescriptors, int maxFramesInFlight)
{
	//TODO: Maybe sort into map by VkShaderStageFlagBits instead of storing each vector as a member

	size_t numUniformBufferDescriptors = uniformBufferDescriptors.size();
	size_t numComputeUniformBufferDescriptors = computeUniformBufferDescriptors.size();
	mTotalUniformBufferSize = 0;
	mTotalComputeUniformBufferSize = 0;

	mBufferToOffset.clear();
	mBufferToOffset.resize(maxFramesInFlight);

	for (int i = 0; i < numUniformBufferDescriptors; i++)
	{
		//Ignores allocating memory for buffer taking data from previous frames (because that data will already be allocated)
		if (computeUniformBufferDescriptors[i]->getSourceFromPastFrame()) { continue; }

		if (uniformBufferDescriptors[i]->getDescriptorSetLayoutBinding().descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			mTotalUniformBufferSize += uniformBufferDescriptors[i]->getDataSize();
		}
		else
		{
			mTotalStorageBufferSize += uniformBufferDescriptors[i]->getDataSize();
		}
	}

	for (int i = 0; i < numComputeUniformBufferDescriptors; i++)
	{
		//Ignores allocating memory for buffer taking data from previous frames (because that data will already be allocated)
		if (computeUniformBufferDescriptors[i]->getSourceFromPastFrame()) { continue; }

		if (computeUniformBufferDescriptors[i]->getDescriptorSetLayoutBinding().descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			mTotalComputeUniformBufferSize += computeUniformBufferDescriptors[i]->getDataSize();
		}
		else
		{
			mTotalComputeStorageBufferSize += computeUniformBufferDescriptors[i]->getDataSize();
		}
	}

	mUniformBufferDescriptors = uniformBufferDescriptors;
	mUniformImageDescriptors = uniformImageDescriptors;

	mComputeUniformBufferDescriptors = computeUniformBufferDescriptors;
	mComputeUniformImageDescriptors = computeUniformImageDescriptors;
}

std::vector<VkDescriptorSetLayoutBinding> DescriptorSetsData::getLayoutBindings()
{
	size_t numUniformBuffers = mUniformBufferDescriptors.size();
	size_t numUniformImages = mUniformImageDescriptors.size();
	int index = 0;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(numUniformBuffers + numUniformImages);

	for (int i = 0; i < numUniformBuffers; i++)
	{
		layoutBindings[index] = mUniformBufferDescriptors[i]->getDescriptorSetLayoutBinding();
		index++;
	}

	for (int i = 0; i < numUniformImages; i++)
	{
		layoutBindings[index] = mUniformImageDescriptors[i]->getDescriptorSetLayoutBinding();
		index++;
	}

	return layoutBindings;
}

std::vector<VkDescriptorSetLayoutBinding> DescriptorSetsData::getComputeLayoutBindings()
{
	size_t numUniformBuffers = mComputeUniformBufferDescriptors.size();
	size_t numUniformImages = mComputeUniformImageDescriptors.size();
	int index = 0;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(numUniformBuffers + numUniformImages);

	for (int i = 0; i < numUniformBuffers; i++)
	{
		layoutBindings[index] = mComputeUniformBufferDescriptors[i]->getDescriptorSetLayoutBinding();
		index++;
	}

	for (int i = 0; i < numUniformImages; i++)
	{
		layoutBindings[index] = mComputeUniformImageDescriptors[i]->getDescriptorSetLayoutBinding();
		index++;
	}

	return layoutBindings;
}

std::vector<VkWriteDescriptorSet> DescriptorSetsData::getWriteDescriptorSets(VkDescriptorSet destSet, 
	std::vector<VkBuffer> destUniformBuffers, std::vector<VkBuffer> destStorageBuffers, size_t currFrame)
{
	size_t numUniformBuffers = mUniformBufferDescriptors.size();
	size_t numUniformImages = mUniformImageDescriptors.size();
	int index = 0;

	std::vector<VkWriteDescriptorSet> writes(numUniformBuffers + numUniformImages);

	for (int i = 0; i < numUniformBuffers; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.range = mUniformBufferDescriptors[i]->getDataSize();

		size_t sourceFrame = mUniformBufferDescriptors[i]->getSourceFromPastFrame() ? 
			(currFrame - 1) % destStorageBuffers.size() : currFrame;

		if (mUniformBufferDescriptors[i]->getDescriptorSetLayoutBinding().descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			bufferInfo.buffer = destStorageBuffers[sourceFrame];
		}
		else
		{
			bufferInfo.buffer = destUniformBuffers[sourceFrame];
		}

		if (mBufferToOffset[currFrame].find(bufferInfo.buffer) == mBufferToOffset[currFrame].end()) { 
			mBufferToOffset[currFrame][bufferInfo.buffer] = 0; }

		bufferInfo.offset = mBufferToOffset[currFrame][bufferInfo.buffer];

		mUniformBufferDescriptors[i]->setBufferInfo(bufferInfo);
		writes[index] = mUniformBufferDescriptors[i]->getWriteDescriptorSet(destSet);


		mBufferToOffset[currFrame][bufferInfo.buffer] += bufferInfo.range;
		index++;
	}

	for (int i = 0; i < numUniformImages; i++)
	{
		writes[index] = mUniformImageDescriptors[i]->getWriteDescriptorSet(destSet);
		index++;
	}

	return writes;
}

std::vector<VkWriteDescriptorSet> DescriptorSetsData::getComputeWriteDescriptorSets(VkDescriptorSet destSet,
	std::vector<VkBuffer> destUniformBuffers, std::vector<VkBuffer> destStorageBuffers, size_t currFrame)
{
	size_t numUniformBuffers = mComputeUniformBufferDescriptors.size();
	size_t numUniformImages = mComputeUniformImageDescriptors.size();
	int index = 0;

	std::vector<VkWriteDescriptorSet> writes(numUniformBuffers + numUniformImages);

	for (int i = 0; i < numUniformBuffers; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.range = mComputeUniformBufferDescriptors[i]->getDataSize();

		size_t sourceFrame = mComputeUniformBufferDescriptors[i]->getSourceFromPastFrame() ?
			(currFrame - 1) % destStorageBuffers.size() : currFrame;

		if (mComputeUniformBufferDescriptors[i]->getDescriptorSetLayoutBinding().descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			bufferInfo.buffer = destStorageBuffers[sourceFrame];
		}
		else
		{
			bufferInfo.buffer = destUniformBuffers[sourceFrame];
		}

		if (mBufferToOffset[currFrame].find(bufferInfo.buffer) == mBufferToOffset[currFrame].end()) {
			mBufferToOffset[currFrame][bufferInfo.buffer] = 0;
		}

		bufferInfo.offset = mBufferToOffset[currFrame][bufferInfo.buffer];

		mComputeUniformBufferDescriptors[i]->setBufferInfo(bufferInfo);
		writes[index] = mComputeUniformBufferDescriptors[i]->getWriteDescriptorSet(destSet);

		mBufferToOffset[currFrame][bufferInfo.buffer] += bufferInfo.range;
		index++;
	}

	for (int i = 0; i < numUniformImages; i++)
	{
		writes[index] = mComputeUniformImageDescriptors[i]->getWriteDescriptorSet(destSet);
		index++;
	}

	return writes;
}

void DescriptorSetsData::updateBufferUniforms(void* destBuffer)
{
	size_t numObjects = mUniformBufferDescriptors.size();

	for (int i = 0; i < numObjects; i++)
	{
		if (mUniformBufferDescriptors[i]->getUniformType() != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) 
		{
			memcpy(destBuffer, mUniformBufferDescriptors[i]->getDataPointer(), mUniformBufferDescriptors[i]->getDataSize());
		}
	}
}

void DescriptorSetsData::updateComputeBufferUniforms(void* destBuffer)
{
	size_t numObjects = mComputeUniformBufferDescriptors.size();

	for (int i = 0; i < numObjects; i++)
	{
		//TODO: Add a better way of handling storage buffers vs uniform buffers
		if (mComputeUniformBufferDescriptors[i]->getUniformType() != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			//This is to add the offset of the actual data stored in the uniform buffer (so as to not use vkMapMemory for each uniform)
			uint8_t* bytePointer = static_cast<uint8_t*>(destBuffer);

			memcpy(bytePointer + mComputeUniformBufferDescriptors[i]->getOffset(), 
				mComputeUniformBufferDescriptors[i]->getDataPointer(), mComputeUniformBufferDescriptors[i]->getDataSize());
		}
	}
}
