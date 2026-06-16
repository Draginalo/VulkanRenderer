#include "Pipeline.h"

uint32_t Pipeline::mNumPipelineInstances = 0;

std::vector<char> Pipeline::readShaderFile(const char* filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	std::vector<char> buffer = {};

	if (!file.is_open())
	{
		std::cout << "\nFailed to open file: " << filepath << std::endl;
		return {};
	}

	size_t fileSize = (size_t)file.tellg();
	buffer.resize(fileSize);

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