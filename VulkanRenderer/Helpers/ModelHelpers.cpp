#include "ModelHelpers.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

bool loadModel(const char* filepath, std::vector<Vertex3D>* vertices, std::vector<uint32_t>* indecies)
{
	tinyobj::attrib_t attrib;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning;
	std::string error;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filepath))
	{
		std::cout << "\nFailed to load obj file: " << filepath << std::endl << "With errors: " << error << std::endl;
		return false;
	}

	std::unordered_map<Vertex3D, uint32_t> uniqueVertices{};

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex3D vertex{};
			vertex.position = {
				attrib.vertices[static_cast<uint32_t>(3 * index.vertex_index)],
				attrib.vertices[static_cast<uint32_t>(3 * index.vertex_index + 1)],
				attrib.vertices[static_cast<uint32_t>(3 * index.vertex_index + 2)]
			};

			vertex.texCoords = {
				attrib.texcoords[static_cast<uint32_t>(2 * index.texcoord_index)],
				1.0f - attrib.texcoords[static_cast<uint32_t>(2 * index.texcoord_index + 1)]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			//Does not add duplicate verticies (records index of that first recorded vertex instead)
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices->size());
				vertices->push_back(vertex);
			}

			indecies->push_back(uniqueVertices[vertex]);
		}
	}

	return true;
}
