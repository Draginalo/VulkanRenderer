#pragma once

#include <vector>
#include <iostream>

#include "VulkanRenderer/Mesh/MeshGeneric.h"

bool loadModel(const char* filepath, std::vector<Vertex3D>* vertices, std::vector<uint32_t>* indecies);