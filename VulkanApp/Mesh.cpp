#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unordered_map>

#include "RenderPass.h"
#include "Logging.h"

std::vector<Mesh::Vertex> Mesh::vertices;
std::vector<uint32_t> Mesh::indices;

Mesh::Mesh(const char* obj)
{
	if (vertices.size() > 0)
		return;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, obj))
		THROW("failed to load obj with error: " + err)

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	uniqueVertices.reserve(attrib.vertices.size());
	vertices.reserve(attrib.vertices.size());
	indices.reserve(attrib.vertices.size());
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
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

Mesh::~Mesh()
{
	
}

void Mesh::init(const Device&)
{
}

void Mesh::clean(const Device& device)
{
	vkDestroyBuffer(device.getDevice(), _uniformBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _uniformBufferMemory, nullptr);

	vkDestroyBuffer(device.getDevice(), _indexBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _indexBufferMemory, nullptr);

	vkDestroyBuffer(device.getDevice(), _vertexBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _vertexBufferMemory, nullptr);

	vkDestroySampler(device.getDevice(), _textureSampler, nullptr);
	vkDestroyImageView(device.getDevice(), _textureImageView, nullptr);

	vkDestroyImage(device.getDevice(), _textureImage, nullptr);
	vkFreeMemory(device.getDevice(), _textureImageMemory, nullptr);
}

void Mesh::update(const Device& device, float deltaTime, float d)
{
	static float l = 0.f;
	l += deltaTime;

	RenderPass::UniformBufferObject ubo = {};
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(d * 2.f, 5.f, 0.f)) * glm::rotate(glm::mat4(1.f), l * glm::radians(45.f), glm::vec3(0.f, 0.f, 1.f));
	ubo.view = glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
	ubo.proj = glm::perspective(glm::radians(45.f), 800.f / 600.f, 0.1f, 100.f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device.getDevice(), _uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device.getDevice(), _uniformBufferMemory);
}
