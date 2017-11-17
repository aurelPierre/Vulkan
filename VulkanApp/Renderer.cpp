#include "Renderer.h"

#include "Logging.h"

void Renderer::init(const Device& device, VkSurfaceKHR surface)
{
	createSwapChain(device, surface);
	_renderPass.init(device, _swapChainImageFormat, _swapChainExtent);
	initImages(device);
}

void Renderer::initImages(const Device& device)
{
	createImageViews(device);
	createFramebuffers(device);
	createCommandPool(device);
	createCommandBuffers(device);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphore);
	if (result != VK_SUCCESS)
		THROW("failed to create semaphore with error: " + result)

		result = vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphore);
	if (result != VK_SUCCESS)
		THROW("failed to create semaphore with error: " + result)
}

void Renderer::clean(const Device& device)
{
	vkDestroySemaphore(device.getDevice(), _renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device.getDevice(), _imageAvailableSemaphore, nullptr);

	vkDestroyCommandPool(device.getDevice(), _commandPool, nullptr);

	for (auto& image : _images)
		vkDestroyFramebuffer(device.getDevice(), image._framebuffer, nullptr);

	_renderPass.clean(device);

	for (auto& image : _images)
		vkDestroyImageView(device.getDevice(), image._imageView, nullptr);

	vkDestroySwapchainKHR(device.getDevice(), _swapChain, nullptr);
}

void Renderer::draw(const Device& device)
{
	vkQueueWaitIdle(device.getPresentQueue());

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device.getDevice(), _swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_images[imageIndex]._commandBuffer;

	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult result = vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		THROW("failed to submit draw command buffer with error: " + result)

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
}

void Renderer::createSwapChain(const Device& device, VkSurfaceKHR surface)
{
	Device::SwapChainSupportDetails swapChainSupport = device.getSwapChainSupportDetails();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport._formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport._presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport._capabilities);

	uint32_t imageCount = swapChainSupport._capabilities.minImageCount + 1;
	if (swapChainSupport._capabilities.maxImageCount > 0 && imageCount > swapChainSupport._capabilities.maxImageCount)
		imageCount = swapChainSupport._capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	Device::QueueFamilyIndices indices = device.getIndices();
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport._capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &_swapChain);
	if (result != VK_SUCCESS)
		THROW("failed to create swap chain with error:" + result)

	_swapChainImageFormat = surfaceFormat.format;
	_swapChainExtent = extent;

	vkGetSwapchainImagesKHR(device.getDevice(), _swapChain, &imageCount, nullptr);
	_images.resize(imageCount);
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(device.getDevice(), _swapChain, &imageCount, images.data());

	for (size_t i = 0; i < images.size(); ++i)
		_images[i]._image = images[i];
}

void Renderer::createImageViews(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _images[i]._image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(device.getDevice(), &createInfo, nullptr, &_images[i]._imageView);
		if (result != VK_SUCCESS)
			THROW("failed to create image view with error: " + result)
	}
}

void Renderer::createFramebuffers(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i) {
		VkImageView attachments[] = {
			_images[i]._imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass.getRenderPass();
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &_images[i]._framebuffer);
		if (result != VK_SUCCESS)
			THROW("failed to create framebuffer with error: " + result)
	}
}

void Renderer::createCommandPool(const Device& device)
{
	Device::QueueFamilyIndices queueFamilyIndices = device.getIndices();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0;

	VkResult result = vkCreateCommandPool(device.getDevice(), &poolInfo, nullptr, &_commandPool);
	if (result != VK_SUCCESS)
		THROW("failed to create command pool with error: " + result)
}

void Renderer::createCommandBuffers(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &_images[i]._commandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to allocate command buffers with error: " + result)

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(_images[i]._commandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass.getRenderPass();
		renderPassInfo.framebuffer = _images[i]._framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChainExtent;

		VkClearValue clearColor = { 0.15f, 0.15f, 0.15f, 1.f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(_images[i]._commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(_images[i]._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipeline());
		vkCmdDraw(_images[i]._commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(_images[i]._commandBuffer);

		result = vkEndCommandBuffer(_images[i]._commandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to record command buffer with error: " + result)
	}
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;

	return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		VkExtent2D actualExtent = { 800, 600 };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}
