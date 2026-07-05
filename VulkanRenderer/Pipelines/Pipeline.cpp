#include "Pipeline.h"

uint32_t Pipeline::mNumPipelineInstances = 0;

Pipeline::Pipeline(bool isComputePipeline) : mPipelineID(mNumPipelineInstances++), mIsComputePipeline(isComputePipeline) {}

std::vector<char> Pipeline::readShaderFile(const char* filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	std::vector<char> buffer = {};

	if (!file.is_open())
	{
		std::cout << "\nFailed to open file: " << filepath << std::endl;
		return {};
	}

	std::streamsize fileSize = (std::streamsize)file.tellg();
	buffer.resize(static_cast<size_t>(fileSize));

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule Pipeline::createShaderModule(VkDevice logicalDevice, const std::vector<char>& shaderByteCode)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = shaderByteCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderByteCode.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create shader module..." << std::endl;
		return nullptr;
	}

	return shaderModule;
}

void Pipeline::loadPipelineDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors, 
	std::vector<UniformImageDescriptor> uniformImageDescriptors)
{
	mPipelineDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors);
}

void Pipeline::loadBaseMaterialDescriptorSetData(std::vector<UniformBufferDescriptor> uniformBufferDescriptors, 
	std::vector<UniformImageDescriptor> uniformImageDescriptors)
{
	mBaseMaterial.materialDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors);
}

void Pipeline::createPipelineDescriptorSetLayout(VkDevice logicalDevice)
{ mPipelineDescriptorSetData.createDescriptorSetLayout(logicalDevice); }

void Pipeline::createBaseMaterialDescriptorSetLayout(VkDevice logicalDevice)
{
	//Only creates the descriptor set data if there are descriptors to add
	if (mBaseMaterial.materialDescriptorSetData.getTotalDescriptorsForMaterial() != 0)
	{
		mBaseMaterial.materialDescriptorSetData.createDescriptorSetLayout(logicalDevice);
	}
}

bool Pipeline::createBaseMaterialDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool, BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight)
{
	mBaseMaterial.pipelineForMaterial = this;

	//Only creates the descriptor set data if there are descriptors to add
	if (mBaseMaterial.materialDescriptorSetData.getTotalDescriptorsForMaterial() == 0) { return true; }

	return mBaseMaterial.materialDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool, destUniformBuffers,
		destStorageBuffers, maxFramesInFlight);
}

bool Pipeline::createPipelineDescriptorSetData(VkDevice logicalDevice, VkDescriptorPool descriptorPool, BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight)
{
	return mPipelineDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool, destUniformBuffers, destStorageBuffers,
		maxFramesInFlight);
}

void Pipeline::createPipelineMaterial(std::vector<UniformBufferDescriptor> uniformBufferDescriptors,
	std::vector<UniformImageDescriptor> uniformImageDescriptors, VkDevice logicalDevice, VkDescriptorPool descriptorPool,
	BufferData* destUniformBuffers, BufferData* destStorageBuffers, uint32_t maxFramesInFlight)
{
	for (const UniformBufferDescriptor& baseBufferDescriptor : (*mBaseMaterial.materialDescriptorSetData.getUniformBufferDescriptors()))
	{
        std::vector<int> t;
		if (std::find(uniformBufferDescriptors.begin(), uniformBufferDescriptors.end(), baseBufferDescriptor) ==
                uniformBufferDescriptors.end())
		{
			throw std::runtime_error("New material does not follow base material buffer descriptor types");
		}
	}

	if (mBaseMaterial.materialDescriptorSetData.getUniformBufferDescriptors()->size() != uniformBufferDescriptors.size())
	{
		throw std::runtime_error("New material contains duplicate buffer descriptors");
	}

	for (const UniformImageDescriptor& baseImageDescriptor : (*mBaseMaterial.materialDescriptorSetData.getUniformImageDescriptors()))
	{
		if (std::find(uniformImageDescriptors.begin(), uniformImageDescriptors.end(), baseImageDescriptor) ==
			uniformImageDescriptors.end())
		{
			throw std::runtime_error("New material does not follow base material image descriptor types");
		}
	}

	if (mBaseMaterial.materialDescriptorSetData.getUniformImageDescriptors()->size() != uniformImageDescriptors.size())
	{
		throw std::runtime_error("New material contains duplicate buffer descriptors");
	}

	size_t newMatIndex = mPipelineMaterials.size();
	Material newMat{};
	newMat.pipelineForMaterial = this;

	mPipelineMaterials.push_back(newMat);

	mPipelineMaterials[newMatIndex].materialDescriptorSetData.loadDescriptors(uniformBufferDescriptors, uniformImageDescriptors);

	mPipelineMaterials[newMatIndex].materialDescriptorSetData.createDescriptorSetData(logicalDevice, descriptorPool,
		destUniformBuffers, destStorageBuffers, maxFramesInFlight);
}

void Pipeline::cleanupPipeline(VkDevice logicalDevice)
{
	vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
	mPipelineDescriptorSetData.cleanup(logicalDevice);

	mBaseMaterial.materialDescriptorSetData.cleanup(logicalDevice);

	for (const Material& material : mPipelineMaterials)
	{
		material.materialDescriptorSetData.cleanup(logicalDevice);
	}

	mPipelineMaterials.clear();
}

//Inline one liners
VkPipeline Pipeline::getPipeline() const { return mPipeline; }
VkPipelineLayout Pipeline::getPipelineLayout() const { return mPipelineLayout; }
const std::vector<Material>* Pipeline::getPipelineMaterials() const { return &mPipelineMaterials; }
const Material* Pipeline::getBaseMaterial() const { return &mBaseMaterial; }
const DescriptorSetData* Pipeline::getPipelineDescriptorSetData() const { return &mPipelineDescriptorSetData; }
const UniformBufferDescriptor* Pipeline::getPipelineBufferDescriptor(uint32_t index) const
{ return &(*mPipelineDescriptorSetData.getUniformBufferDescriptors())[index]; }
UniformBufferDescriptor* Pipeline::getPipelineBufferDescriptorRef(uint32_t index)
{ return &(*mPipelineDescriptorSetData.getUniformBufferDescriptorsRef())[index]; }
uint32_t Pipeline::getPipelineID() const { return mPipelineID; }
bool Pipeline::getIsComputePipeline() const { return mIsComputePipeline; }
void Pipeline::setDependencyInfo(PipelineDependencyInfo dependencyInfo) { mDependencyInfo = dependencyInfo; }
PipelineDependencyInfo* Pipeline::getPipelineDependencyInfo() { return &mDependencyInfo; }