#include "Mesh.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unordered_map>

#include "RenderPass.h"
#include "Logging.h"
#include "App.h"

Mesh::Mesh(Model* model, Texture* texture) : _model { model }, _texture { texture }
{
}

Mesh::~Mesh()
{	
}

void Mesh::init(const Device&)
{
}

void Mesh::clean(const Device& device)
{
	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), _uniformBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), _uniformBufferMemory, nullptr);
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
	vkMapMemory(core::App::instance().getDevice().getDevice(), _uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(core::App::instance().getDevice().getDevice(), _uniformBufferMemory);
}
