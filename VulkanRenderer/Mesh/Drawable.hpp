#pragma once

#include <vulkan/vulkan.h>

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const = 0;
	virtual VkBuffer getLastFrameVertexBuffer() const = 0;
private:
	//Explicit padding for compiler warning, since this class' size is not divisable by 8 in x86 and 
	// member variables in derived class have alignment of 8 
	#if (defined(_M_IX86) || defined(__i386__))
		uint8_t padding[4] = {};
	#endif
};