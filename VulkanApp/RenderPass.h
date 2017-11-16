#pragma once

#include "Device.h"
#include "SwapChain.h"

class RenderPass
{
public:
	RenderPass() = default;
	~RenderPass() = default;

	void init(const Device&, const SwapChain&);
	void clean(const Device&);

	VkRenderPass getRenderPass() const { return _renderPass; }
	VkPipeline getPipeline() const { return _graphicsPipeline; }

private:
	static std::vector<char> readFile(const std::string& filename);

	void createRenderPass(const Device&, const SwapChain&);
	void createGraphicsPipeline(const Device&, const SwapChain&);

	VkShaderModule createShaderModule(const std::vector<char>& code, const Device&);

	VkRenderPass _renderPass;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicsPipeline;
};

