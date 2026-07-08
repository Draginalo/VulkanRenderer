#pragma once

#include "Drawable.hpp"
#include <vulkan/vulkan.h>
#include <vector>

struct BufferDrawerData {
	VkDeviceSize offset;
	std::vector<VkBuffer>* framesInFlightBuffers;
	uint32_t numVertices;

	//Explicit padding for compiler warning, since this struct's size is not divisable by 8 in x64
	#if defined(_WIN64) || defined(__x86_64__)
		uint8_t padding[4] = {};
	#endif
};

class BufferDrawer : public Drawable {
public:
	void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const override;
    VkBuffer getLastFrameVertexBuffer() const override;
	void setBufferDrawData(BufferDrawerData bufferData);
private:
	BufferDrawerData mBufferDrawData = {};
};