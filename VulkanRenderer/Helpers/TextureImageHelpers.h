#pragma once

#include <iostream>
#include "vulkan/vulkan.h"

bool createTextureImage(VkImage image, VkDeviceSize imageMemmory, const char* filename);