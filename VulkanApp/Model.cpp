#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

#include "Logging.h"
#include "App.h"

Model::Model(std::string file)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file.c_str()))
		THROW("failed to load obj with error: " + err)

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	uniqueVertices.reserve(attrib.vertices.size());
	_vertices.reserve(attrib.vertices.size());
	_indices.reserve(attrib.vertices.size());
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.f, 1.f, 1.f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
				_vertices.push_back(vertex);
			}

			_indices.push_back(uniqueVertices[vertex]);
		}
	}

	createVertexBuffer();
	createIndexBuffer();
}

Model::~Model()
{
	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), _indexBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), _indexBufferMemory, nullptr);

	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), _vertexBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), _vertexBufferMemory, nullptr);
}

void Model::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory staginBufferMemory;
	core::App::instance().getRenderer().createBuffer(core::App::instance().getDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginBufferMemory);

	void* data;
	vkMapMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory);

	core::App::instance().getRenderer().createBuffer(core::App::instance().getDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, _vertexBuffer, _vertexBufferMemory);

	core::App::instance().getRenderer().copyBuffer(core::App::instance().getDevice(), stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory, nullptr);
}

void Model::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory staginBufferMemory;
	core::App::instance().getRenderer().createBuffer(core::App::instance().getDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginBufferMemory);

	void* data;
	vkMapMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory);

	core::App::instance().getRenderer().createBuffer(core::App::instance().getDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_indexBuffer, _indexBufferMemory);

	core::App::instance().getRenderer().copyBuffer(core::App::instance().getDevice(), stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), staginBufferMemory, nullptr);
}
