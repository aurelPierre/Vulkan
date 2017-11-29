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
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, uv);

			return attributeDescriptions;
		}
	};

	RenderPass() = default;
	~RenderPass() = default;

	void init(const Device&, VkFormat, VkExtent2D);
	void clean(const Device&);
	VkFormat findDepthFormat(const Device&);

	VkRenderPass getRenderPass() const { return _renderPass; }
	VkPipeline getPipeline() const { return _graphicsPipeline; }
	VkPipelineLayout getPipelineLayout() const { return _pipelineLayout; }
	VkDescriptorSetLayout getDescriptorSetLayout() const { return _descriptorSetLayout; }

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

