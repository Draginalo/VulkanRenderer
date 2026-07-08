#pragma once

#include <vector>
#include <iostream>

#include "VulkanRenderer/Helpers/VertexInputData.h"

bool loadModel(const char* filepath, std::vector<Vertex3D>* vertices, std::vector<uint32_t>* indecies);