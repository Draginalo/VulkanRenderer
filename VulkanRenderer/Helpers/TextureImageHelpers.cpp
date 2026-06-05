#include "TextureImageHelpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool createTextureImage(VkImage image, VkDeviceSize imageMemmory, const char* filename)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		std::cout << "\nFailed to load image file: " << filename << "..." << std::endl;
		return false;
	}

	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;

	return true;
}
