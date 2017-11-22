#include "Renderer.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "App.h"
#include "Logging.h"

void Renderer::init(const Device& device, VkSurfaceKHR surface)
{
	vkGetDeviceQueue(device.getDevice(), device.getIndices().presentFamily, 0, &_presentQueue);
	vkGetDeviceQueue(device.getDevice(), device.getIndices().presentFamily, 0, &_graphicsQueue);

	createSwapChain(device, surface);
	_renderPass.init(device, _swapChainImageFormat, _swapChainExtent);
	initImages(device);
}

void Renderer::initImages(const Device& device)
{
	createImageViews(device);
	createFramebuffers(device);
	createCommandPool(device);
	createVertexBuffer(device);
	createIndexBuffer(device);
	createUniformBuffer(device);
	createDescriptorPool(device);
	createDescriptorSet(device);
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

	vkDestroyDescriptorPool(device.getDevice(), _descriptorPool, nullptr);

	vkDestroyBuffer(device.getDevice(), _uniformBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _uniformBufferMemory, nullptr);

	vkDestroyBuffer(device.getDevice(), _indexBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _indexBufferMemory, nullptr);

	vkDestroyBuffer(device.getDevice(), _vertexBuffer, nullptr);
	vkFreeMemory(device.getDevice(), _vertexBufferMemory, nullptr);

	vkDestroyCommandPool(device.getDevice(), _commandPool, nullptr);

	for (auto& image : _images)
		vkDestroyFramebuffer(device.getDevice(), image._framebuffer, nullptr);

	_renderPass.clean(device);

	for (auto& image : _images)
		vkDestroyImageView(device.getDevice(), image._imageView, nullptr);

	vkDestroySwapchainKHR(device.getDevice(), _swapChain, nullptr);
	_images.clear();
}

void Renderer::recreate(const Device& device, VkSurfaceKHR surface)
{
	vkDeviceWaitIdle(device.getDevice());
	clean(device);
	init(device, surface);
}

void Renderer::update(const Device& device, float deltaTime)
{
	static float l = 0.f;
	l += deltaTime;

	RenderPass::UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.f), l * glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
	ubo.view = glm::lookAt(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	ubo.proj = glm::perspective(glm::radians(90.f), (float)_swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device.getDevice(), _uniformBufferMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device.getDevice(), _uniformBufferMemory);
}

void Renderer::draw(const Device& device)
{
	vkQueueWaitIdle(_presentQueue);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device.getDevice(), _swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		util::Singleton<core::App>::instance().resize();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		THROW("failed to acquire swap chain image with error: " + result)

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

	result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
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

	result = vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		util::Singleton<core::App>::instance().resize();
	else if(result != VK_SUCCESS)
		THROW("failed to present swap chain image with error: " + result)
}

void Renderer::createSwapChain(const Device& device, VkSurfaceKHR surface)
{
	Device::SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(device.getPhysicalDevice(), surface);

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

		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_images[i]._commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(_images[i]._commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(_images[i]._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipelineLayout(), 0, 1,
			&_descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(_images[i]._commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(_images[i]._commandBuffer);

		result = vkEndCommandBuffer(_images[i]._commandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to record command buffer with error: " + result)
	}
}

void Renderer::createUniformBuffer(const Device& device)
{
	VkDeviceSize bufferSize = sizeof(RenderPass::UniformBufferObject);
	createBuffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffer, _uniformBufferMemory);
}

void Renderer::createDescriptorPool(const Device& device)
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	VkResult result = vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &_descriptorPool);
	if(result != VK_SUCCESS)
		THROW("failed to create descriptor pool with error: " + result)
}

void Renderer::createDescriptorSet(const Device& device)
{
	VkDescriptorSetLayout layouts[] = { _renderPass.getDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &_descriptorSet);
	if (result != VK_SUCCESS)
		THROW("failed to allocate descriptor set with error: " + result)

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = _uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(RenderPass::UniformBufferObject);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = _descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
	descriptorWrite.pImageInfo = nullptr;
	descriptorWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void Renderer::createIndexBuffer(const Device& device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory staginBufferMemory;
	createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginBufferMemory);

	void* data;
	vkMapMemory(device.getDevice(), staginBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device.getDevice(), staginBufferMemory);

	createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				_indexBuffer, _indexBufferMemory);

	copyBuffer(device, stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device.getDevice(), staginBufferMemory, nullptr);
}

void Renderer::createVertexBuffer(const Device& device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory staginBufferMemory;
	createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
				| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginBufferMemory);

	void* data;
	vkMapMemory(device.getDevice(), staginBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device.getDevice(), staginBufferMemory);

	createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				, _vertexBuffer, _vertexBufferMemory);

	copyBuffer(device, stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device.getDevice(), staginBufferMemory, nullptr);
}

void Renderer::createBuffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, 
							VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &buffer);
	if (result != VK_SUCCESS)
		THROW("failed to create vertex buffer with error: " + result)

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device.getDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
	
	result = vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &memory);
	if (result != VK_SUCCESS)
		THROW("failed to allocate vertex buffer memory with errror: " + result)

	vkBindBufferMemory(device.getDevice(), buffer, memory, 0);
}

void Renderer::copyBuffer(const Device& device, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(device.getDevice(), _commandPool, 1, &commandBuffer);
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
		int width, height;
		util::Singleton<core::App>::instance().getWindowSize(width, height);
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}
