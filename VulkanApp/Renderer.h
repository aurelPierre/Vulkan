#pragma once

#include <vulkan\vulkan.h>
#include <vector>

#include "NonCopyable.h"
#include "Device.h"
#include "RenderPass.h"

class Renderer : public util::NonCopyable
{
private:
	struct Image {
		VkImage _image;
		VkImageView _imageView;
		VkFramebuffer _framebuffer;
		VkCommandBuffer _commandBuffer;
	};

public:
	Renderer() = default;
	~Renderer() = default;

	void init(const Device&, VkSurfaceKHR);
	void initImages(const Device&);
	void clean(const Device&);

	void draw(const Device&);

	VkSwapchainKHR getSwapchainKHR() const { return _swapChain; }
	VkFormat getFormat() const { return _swapChainImageFormat; }
	const VkExtent2D& getExtent() const { return _swapChainExtent; }
	const VkCommandBuffer& getCommandBuffer(uint32_t index) { return _images[index]._commandBuffer; }

private:
	void createSwapChain(const Device&, VkSurfaceKHR);
	void createImageViews(const Device&);

	void createFramebuffers(const Device&);
	void createCommandPool(const Device&);
	void createCommandBuffers(const Device&);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkSwapchainKHR _swapChain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	RenderPass _renderPass;

	VkCommandPool _commandPool;
	std::vector<Image> _images;

	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;
};

