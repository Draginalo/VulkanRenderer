#pragma once

#include "vulkan/vulkan.h"

class Drawable {
public:
	virtual void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const = 0;
};