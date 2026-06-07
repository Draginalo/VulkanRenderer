#pragma once

#include <vector>
#include <iostream>

#include "../VertexBufferData.h"

bool loadModel(const char* filepath, std::vector<Vertex>* vertices, std::vector<uint32_t>* indecies);