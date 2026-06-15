#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"

#include <iostream>
#include <fstream>
#include <vector>

class ComputePipeline : public Pipeline {
public:
	bool creatPipeline(VkDevice logicalDevice, VkDescriptorSetLayout descriptorSetLayout);
	void bindPipeline(VkCommandBuffer commandBuffer) override;
private:
};