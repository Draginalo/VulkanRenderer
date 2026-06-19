#pragma once

#include "Drawable.h"
#include "vulkan/vulkan.h"
#include <vector>

struct BufferDrawerData {
	std::vector<VkBuffer>* framesInFlightBuffers;
	VkDeviceSize offset;
	uint32_t numVertices;
};

class BufferDrawer : public Drawable {
public:
	void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const override;
	void setBufferDrawData(BufferDrawerData bufferData) { mBufferDrawData = bufferData; }
private:
	BufferDrawerData mBufferDrawData;
};