#include "DescriptorSetData.h"

void DescriptorSetData::loadDescriptors(std::vector<UniformBufferDescriptor> uniformBufferDescriptors, 
	std::vector<UniformImageDescriptor> uniformImageDescriptors)
{
	size_t numUniformBufferDescriptors = uniformBufferDescriptors.size();
	mTotalUniformBufferSize = 0;

	for (size_t i = 0;  (i < numUniformBufferDescriptors) == true; i++)
	{
		//Ignores allocating memory for buffer taking data from previous frames (because that data will already be allocated)
		if (uniformBufferDescriptors[i].getSourceFromPastFrame()) { continue; }

		if (uniformBufferDescriptors[i].getDescriptorSetLayoutBinding().descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			mTotalUniformBufferSize += uniformBufferDescriptors[i].getDataSize();
		}
		else
		{
			mTotalStorageBufferSize += uniformBufferDescriptors[i].getDataSize();
		}
	}

	mTotalDescriptorsForMaterial += static_cast<uint32_t>(numUniformBufferDescriptors + uniformImageDescriptors.size());

	mUniformBufferDescriptors = uniformBufferDescriptors;
	mUniformImageDescriptors = uniformImageDescriptors;
}

bool DescriptorSetData::createDescriptorSetLayout(VkDevice logicalDevice)
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = getLayoutBindings();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	return true;
}

bool DescriptorSetData::createDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool,
	BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight)
{
	std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, mDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo{};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = descriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
	setAllocateInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize(maxFramesInFlight);

	if (vkAllocateDescriptorSets(logicalDevice, &setAllocateInfo, mDescriptorSets.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate descriptor sets for graphics..." << std::endl;
		return false;
	}


	uint32_t destUniformAllocSize = destUniformBuffers->currentlyAllocatedSize;
	uint32_t destStorageAllocSize = destStorageBuffers->currentlyAllocatedSize;
	for (uint32_t i = 0; (i < maxFramesInFlight) == true; i++)
	{
		//Resets allocations for each frame (cuz each frame uses a different buffer)
		destUniformBuffers->currentlyAllocatedSize = destUniformAllocSize;
		destStorageBuffers->currentlyAllocatedSize = destStorageAllocSize;

		std::vector<VkWriteDescriptorSet> descriptorWrites =
			getWriteDescriptorSets(mDescriptorSets[i], destUniformBuffers, destStorageBuffers, i);

		//TODO: Update this to call update descriptor sets for all descriptor sets (the writes are returned from this function)
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

std::vector<VkDescriptorSetLayoutBinding> DescriptorSetData::getLayoutBindings()
{
	size_t numPipelineUniformBuffers = mUniformBufferDescriptors.size();
	size_t numUniformImages = mUniformImageDescriptors.size();
	uint32_t index = 0;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(numPipelineUniformBuffers + numUniformImages);

	for (size_t i = 0;  (i < numPipelineUniformBuffers) == true; i++)
	{
		layoutBindings[index] = mUniformBufferDescriptors[i].getDescriptorSetLayoutBinding();
		index++;
	}

	for (size_t i = 0;  (i < numUniformImages) == true; i++)
	{
		layoutBindings[index] = mUniformImageDescriptors[i].getDescriptorSetLayoutBinding();
		index++;
	}

	return layoutBindings;
}

std::vector<VkWriteDescriptorSet> DescriptorSetData::getWriteDescriptorSets(VkDescriptorSet destSet, BufferData* destUniformBuffers,
	BufferData* destStorageBuffers, uint32_t currFrame)
{
	size_t numPipelineUniformBuffers = mUniformBufferDescriptors.size();
	size_t numUniformImages = mUniformImageDescriptors.size();
	uint32_t index = 0;

	std::vector<VkWriteDescriptorSet> writes(numPipelineUniformBuffers + numUniformImages);

	for (size_t i = 0;  (i < numPipelineUniformBuffers) == true; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.range = mUniformBufferDescriptors[i].getDataSize();

		BufferData* bufferData = mUniformBufferDescriptors[i].getDescriptorSetLayoutBinding().descriptorType ==
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? destStorageBuffers : destUniformBuffers;

		bufferInfo.offset = bufferData->currentlyAllocatedSize;

		size_t sourceFrame = (currFrame - 1) % destStorageBuffers->buffers.size();

		if (!mUniformBufferDescriptors[i].getSourceFromPastFrame())
		{
			sourceFrame = currFrame;
			bufferData->currentlyAllocatedSize += static_cast<uint32_t>(bufferInfo.range);
		}

		bufferInfo.buffer = bufferData->buffers[sourceFrame];

		mUniformBufferDescriptors[i].setBufferInfo(bufferInfo);
		writes[index] = mUniformBufferDescriptors[i].getWriteDescriptorSet(destSet);

		index++;
	}

	for (size_t i = 0; (i < numUniformImages) == true; i++)
	{
		writes[index] = mUniformImageDescriptors[i].getWriteDescriptorSet(destSet);
		index++;
	}

	return writes;
}

void DescriptorSetData::updateBufferUniforms(void* destBuffer) const
{
	size_t numObjects = mUniformBufferDescriptors.size();

	for (size_t i = 0; (i < numObjects) == true; i++)
	{
		if (mUniformBufferDescriptors[i].getUniformType() != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			//Converts pointer to start of destbuffer to a byte array and then offsets it to the proper data segment in the buffer
			uint8_t* bufferDataPointerOffset = static_cast<uint8_t*>(destBuffer) +
				static_cast<size_t>(mUniformBufferDescriptors[i].getOffset());

			memcpy(bufferDataPointerOffset, mUniformBufferDescriptors[i].getDataPointer(), 
				static_cast<size_t>(mUniformBufferDescriptors[i].getDataSize()));
		}
	}
}

//Inline one liners
const std::vector<UniformBufferDescriptor>* DescriptorSetData::getUniformBufferDescriptors() const 
{ return &mUniformBufferDescriptors; }
const std::vector<UniformImageDescriptor>* DescriptorSetData::getUniformImageDescriptors() const 
{ return &mUniformImageDescriptors; }
std::vector<UniformBufferDescriptor>* DescriptorSetData::getUniformBufferDescriptorsRef() 
{ return &mUniformBufferDescriptors; }
VkDeviceSize DescriptorSetData::getTotalUniformBufferSize() const { return mTotalUniformBufferSize; }
VkDeviceSize DescriptorSetData::getTotalStorageBufferSize() const { return mTotalStorageBufferSize; }
const VkDescriptorSet* DescriptorSetData::getDescriptorSet(uint32_t currFrame) const { return &mDescriptorSets[currFrame]; }
VkDescriptorSetLayout* DescriptorSetData::getDescriptorSetLayout() { return &mDescriptorSetLayout; }
uint32_t DescriptorSetData::getTotalDescriptorsForMaterial() const { return mTotalDescriptorsForMaterial; }
void DescriptorSetData::cleanup(VkDevice logicalDevice) const { vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr); }