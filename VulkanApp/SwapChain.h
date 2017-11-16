#pragma once

#include <vulkan\vulkan.h>
#include <vector>

#include "NonCopyable.h"
#include "Device.h"

class SwapChain : public util::NonCopyable
{
public:
	SwapChain() = default;
	~SwapChain() = default;

	void init(const Device&, VkSurfaceKHR);
	void initBuffers(const Device&, VkRenderPass, VkPipeline);
	void cleanBuffers(const Device&);
	void clean(const Device&);

	VkSwapchainKHR getSwapchainKHR() const { return _swapChain; }
	VkFormat getFormat() const { return _swapChainImageFormat; }
	const VkExtent2D& getExtent() const { return _swapChainExtent; }

	const VkCommandBuffer& getCommandBuffer(uint32_t index) { return _commandBuffers[index]; }

private:
	void createSwapChain(const Device&, VkSurfaceKHR);
	void createImageViews(const Device&);

	void createFramebuffers(const Device&, VkRenderPass);
	void createCommandPool(const Device&);
	void createCommandBuffers(const Device&, VkRenderPass, VkPipeline);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkSwapchainKHR _swapChain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;
	VkCommandPool _commandPool;

	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;
	std::vector<VkFramebuffer> _swapChainFramebuffers;
	std::vector<VkCommandBuffer> _commandBuffers;
};

