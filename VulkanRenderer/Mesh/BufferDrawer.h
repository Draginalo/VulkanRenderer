#pragma once

#include "Drawable.h"
#include "vulkan/vulkan.h"
#include <vector>

struct BufferDrawerData {
	std::vector<VkBuffer>* framesInFlightBuffers;
	VkDeviceSize offset;
	uint32_t numVertices;

	//Padding for compiler warning
	uint8_t padding[4];
};

class BufferDrawer : public Drawable {
public:
	void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const override;
    VkBuffer getLastFrameVertexBuffer() const override;
	void setBufferDrawData(BufferDrawerData bufferData);
private:
	BufferDrawerData mBufferDrawData = {};
};