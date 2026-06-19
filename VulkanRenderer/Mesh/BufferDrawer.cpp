#include "BufferDrawer.h"

void BufferDrawer::draw(VkCommandBuffer commandBuffer, uint32_t currFrame) const
{
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(*mBufferDrawData.framesInFlightBuffers)[currFrame], &mBufferDrawData.offset);

	vkCmdDraw(commandBuffer, mBufferDrawData.numVertices, 1, 0, 0);
}
