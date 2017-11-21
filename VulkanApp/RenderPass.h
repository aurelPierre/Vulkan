#pragma once

#include <glm/glm.hpp>
#include <array>

#include "Device.h"

class RenderPass
{
public:
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	RenderPass() = default;
	~RenderPass() = default;

	void init(const Device&, VkFormat, VkExtent2D);
	void clean(const Device&);

	VkRenderPass getRenderPass() const { return _renderPass; }
	VkPipeline getPipeline() const { return _graphicsPipeline; }

private:
	static std::vector<char> readFile(const std::string& filename);

	void createRenderPass(const Device&, VkFormat);
	void createDescriptorSetLayout(const Device&);
	void createGraphicsPipeline(const Device&, VkExtent2D);

	VkShaderModule createShaderModule(const std::vector<char>& code, const Device&);

	VkRenderPass _renderPass;
	VkDescriptorSetLayout _descriptorSetLayout;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicsPipeline;
};

