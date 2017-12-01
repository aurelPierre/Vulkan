#pragma once

#include <glm/glm.hpp>

#include "Device.h"

class RenderPass
{
public:
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
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