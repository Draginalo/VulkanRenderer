#pragma once

#include "vulkan/vulkan.h"

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const = 0;
	virtual VkBuffer getLastFrameVertexBuffer() const = 0;
};