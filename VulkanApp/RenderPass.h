#pragma once

#include "Device.h"

class RenderPass
{
public:
	RenderPass() = default;
	~RenderPass() = default;

	void init(const Device&, VkFormat, VkExtent2D);
	void clean(const Device&);

	VkRenderPass getRenderPass() const { return _renderPass; }
	VkPipeline getPipeline() const { return _graphicsPipeline; }

private:
	static std::vector<char> readFile(const std::string& filename);

	void createRenderPass(const Device&, VkFormat);
	void createGraphicsPipeline(const Device&, VkExtent2D);

	VkShaderModule createShaderModule(const std::vector<char>& code, const Device&);

	VkRenderPass _renderPass;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicsPipeline;
};

