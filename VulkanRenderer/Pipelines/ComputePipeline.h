#pragma once

#include "vulkan/vulkan.h"
#include "Pipeline.h"

#include <iostream>
#include <fstream>
#include <vector>

class ComputePipeline : public Pipeline {
public:
	ComputePipeline() : Pipeline(true) {}

	bool creatPipeline(VkDevice logicalDevice);
	void bindPipeline(VkCommandBuffer commandBuffer) override;
private:
};